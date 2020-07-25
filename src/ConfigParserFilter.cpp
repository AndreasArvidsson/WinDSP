#include "Config.h"
#include "DSP.h"
#include "JsonNode.h"
#include "WinDSPLog.h"
#include "FilterType.h"
#include "CrossoverType.h"
#include "Str.h"
#include "Convert.h"
#include "Audioclient.h" //WAVE_FORMAT_PCM, WAVE_FORMAT_IEEE_FLOAT
#include "Channel.h"

using std::make_unique;

vector<unique_ptr<Filter>> Config::parseFilters(const shared_ptr<JsonNode>& pNode, const string& path, const int outputChannel) const {
    vector<unique_ptr<Filter>> filters;
    //Parse single instance simple filters.
    parseGain(filters, pNode, path);
    parseDelay(filters, pNode, path);

    unique_ptr<FilterBiquad> pFilterBiquad = make_unique<FilterBiquad>(_sampleRate);
    string filtersPath = path;
    const shared_ptr<JsonNode> pFiltersNode = tryGetArrayNode(pNode, "filters", filtersPath);

    //Apply crossovers from basic config to outputs.
    if (outputChannel > -1) {
        applyCrossoversMap(pFilterBiquad.get(), (Channel)outputChannel, pFiltersNode, filtersPath);
    }

    //Parse filters list
    for (size_t i = 0; i < pFiltersNode->size(); ++i) {
        string filterPath = filtersPath;
        const shared_ptr<JsonNode> pFilter = getObjectNode(pFiltersNode, i, filterPath);
        parseFilter(filters, pFilterBiquad.get(), pFilter, filterPath);
    }

    //Use  biquad filter.
    if (!pFilterBiquad->isEmpty()) {
        filters.push_back(move(pFilterBiquad));
    }

    //Compression needs to be last so that it has the final samples to work on.
    parseCompression(filters, pNode, path);

    return filters;
}

void Config::parseGain(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, const string& path) const {
    const double value = tryGetDoubleValue(pNode, "gain", path);
    const bool invert = tryGetBoolValue(pNode, "invert", path);
    //No use in adding zero gain.
    if (value != 0.0 || invert) {
        filters.push_back(make_unique<FilterGain>(value, invert));
    }
}

void Config::parseDelay(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, string path) const {
    const shared_ptr<JsonNode> pFilterNode = tryGetNode(pNode, "delay", path);
    if (pFilterNode->isMissingNode()) {
        return;
    }
    double value;
    double useUnitMeter = false;
    if (pFilterNode->isNumber()) {
        value = pFilterNode->doubleValue();
    }
    else if (pFilterNode->isObject()) {
        value = getDoubleValue(pFilterNode, "value", path);
        useUnitMeter = tryGetBoolValue(pFilterNode, "unitMeter", path);
    }
    else {
        throw Error("Config(%s) - Invalid delay format. Expected number or object", path.c_str());
    }
    //No use in adding zero delay.
    if (value != 0) {
        const uint32_t sampleDelay = FilterDelay::getSampleDelay(_sampleRate, value, useUnitMeter);
        if (sampleDelay > 0) {
            filters.push_back(make_unique<FilterDelay>(_sampleRate, value, useUnitMeter));
        }
        else {
            LOG_WARN("WARNING: Config(%s) - Discarding delay filter with to low value. Can't delay less then one sample", path.c_str());
        }
    }
}

void Config::parseCompression(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, string path) const {
    if (pNode->has("compression")) {
        const shared_ptr<JsonNode> pFilterNode = getObjectNode(pNode, "compression", path);
        const double threshold = getDoubleValue(pFilterNode, "threshold", path);
        const double ratio = getDoubleValue(pFilterNode, "ratio", path);
        const double attack = getDoubleValue(pFilterNode, "attack", path);
        const double release = getDoubleValue(pFilterNode, "release", path);
        const double window = tryGetDoubleValue(pFilterNode, "window", path);
        filters.push_back(make_unique<FilterCompression>(_sampleRate, threshold, ratio, attack, release, window));
    }
}

void Config::parseCancellation(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, string path) const {
    if (pNode->has("cancellation")) {
        const shared_ptr<JsonNode> pFilterNode = getObjectNode(pNode, "cancellation", path);
        const double freq = getDoubleValue(pFilterNode, "freq", path);
        if (pFilterNode->has("gain")) {
            const double gain = getDoubleValue(pFilterNode, "gain", path);
            filters.push_back(make_unique<FilterCancellation>(_sampleRate, freq, gain));
        }
        else {
            filters.push_back(make_unique<FilterCancellation>(_sampleRate, freq));
        }
    }
}

void Config::parseFilter(vector<unique_ptr<Filter>>& filters, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const FilterType type = getFilterType(pFilterNode, "type", path);
    switch (type) {
    case FilterType::LOW_PASS:
    case FilterType::HIGH_PASS:
        parseCrossover(type == FilterType::LOW_PASS, pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::LOW_SHELF:
    case FilterType::HIGH_SHELF:
        parseShelf(type == FilterType::LOW_SHELF, pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::PEQ:
        parsePEQ(pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::BAND_PASS:
        parseBandPass(pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::NOTCH:
        parseNotch(pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::LINKWITZ_TRANSFORM:
        parseLinkwitzTransform(pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::BIQUAD:
        parseBiquad(pFilterBiquad, pFilterNode, path);
        break;
    case FilterType::FIR:
        parseFir(filters, pFilterNode, path);
        break;
    default:
        throw Error("Config(%s) - Unknown filter type '%s'", path.c_str(), FilterTypes::toString(type).c_str());
    };
}

void Config::parseCrossover(const bool isLowPass, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const CrossoverType crossoverType = getCrossoverType(pFilterNode, path);
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const uint8_t order = (uint8_t)getIntValue(pFilterNode, "order", path);
    switch (crossoverType) {
    case CrossoverType::BUTTERWORTH:
    case CrossoverType::LINKWITZ_RILEY:
    case CrossoverType::BESSEL:
        pFilterBiquad->addCrossover(isLowPass, freq, crossoverType, order, getQOffset(pFilterNode, path));
        break;
    case CrossoverType::CUSTOM:
        pFilterBiquad->addCrossover(isLowPass, freq, getQValues(pFilterNode, order, path));
        break;
    default:
        throw Error("Config(%s) - Unknown crossover sub type %d", path.c_str(), CrossoverTypes::toString(crossoverType).c_str());
    }
}

void Config::parseShelf(const bool isLowShelf, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const double gain = getDoubleValue(pFilterNode, "gain", path);
    if (pFilterNode->has("q")) {
        const double q = getDoubleValue(pFilterNode, "q", path);
        pFilterBiquad->addShelf(isLowShelf, freq, gain, q);
    }
    else {
        pFilterBiquad->addShelf(isLowShelf, freq, gain);
    }
}

void Config::parsePEQ(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const double gain = getDoubleValue(pFilterNode, "gain", path);
    const double q = getDoubleValue(pFilterNode, "q", path);
    pFilterBiquad->addPEQ(freq, gain, q);
}

void Config::parseBandPass(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const double bandwidth = getDoubleValue(pFilterNode, "bandwidth", path);
    if (pFilterNode->has("gain")) {
        const double gain = getDoubleValue(pFilterNode, "gain", path);
        pFilterBiquad->addBandPass(freq, bandwidth, gain);
    }
    else {
        pFilterBiquad->addBandPass(freq, bandwidth);
    }
}

void Config::parseNotch(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const double bandwidth = getDoubleValue(pFilterNode, "bandwidth", path);
    if (pFilterNode->has("gain")) {
        const double gain = getDoubleValue(pFilterNode, "gain", path);
        pFilterBiquad->addNotch(freq, bandwidth, gain);
    }
    else {
        pFilterBiquad->addNotch(freq, bandwidth);
    }
}

void Config::parseLinkwitzTransform(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    const double f0 = getDoubleValue(pFilterNode, "f0", path);
    const double q0 = getDoubleValue(pFilterNode, "q0", path);
    const double fp = getDoubleValue(pFilterNode, "fp", path);
    const double qp = getDoubleValue(pFilterNode, "qp", path);
    pFilterBiquad->addLinkwitzTransform(f0, q0, fp, qp);
}

void Config::parseBiquad(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, string path) const {
    const shared_ptr<JsonNode> pValues = getArrayNode(pFilterNode, "values", path);
    for (size_t i = 0; i < pValues->size(); ++i) {
        string myPath = path;
        const shared_ptr<JsonNode> pValueNode = getObjectNode(pValues, i, myPath);
        const double b0 = getDoubleValue(pValueNode, "b0", myPath);
        const double b1 = getDoubleValue(pValueNode, "b1", myPath);
        const double b2 = getDoubleValue(pValueNode, "b2", myPath);
        const double a1 = getDoubleValue(pValueNode, "a1", myPath);
        const double a2 = getDoubleValue(pValueNode, "a2", myPath);
        if (pValueNode->has("a0")) {
            const double a0 = getDoubleValue(pValueNode, "a0", myPath);
            pFilterBiquad->add(b0, b1, b2, a0, a1, a2);
        }
        else {
            pFilterBiquad->add(b0, b1, b2, a1, a2);
        }
    }
}

void Config::parseFir(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    string myPath = path;
    //Read each line in fir parameter file
    const string filePath = getTextValue(pFilterNode, "file", myPath);
    const File file(filePath);
    const string extension = file.getExtension();
    if (extension.compare("txt") == 0) {
        parseFirTxt(filters, file, myPath);
    }
    else if (extension.compare("wav") == 0) {
        parseFirWav(filters, file, myPath);
    }
    else {
        throw Error("Config(%s) - Unknown file extension for FIR file '%s'", myPath.c_str(), file.getPath().c_str());
    }
}

void Config::parseFirTxt(vector<unique_ptr<Filter>>& filters, const File& file, const string& path) const {
    vector<string> lines;
    if (!file.getData(lines)) {
        throw Error("Config(%s) - Can't read FIR file '%s'", path.c_str(), file.getPath().c_str());
    }
    if (!lines.size()) {
        throw Error("Config(%s) - FIR file '%s' is empty", path.c_str(), file.getPath().c_str());
    }
    vector<double> taps;
    for (const string& str : lines) {
        taps.push_back(atof(str.c_str()));
    }
    filters.push_back(make_unique<FilterFir>(taps));
}

void Config::parseFirWav(vector<unique_ptr<Filter>>& filters, const File& file, const string& path) const {
    unique_ptr<char[]> pBuffer;
    //Get data
    const size_t bufferSize = file.getData(&pBuffer);
    if (!bufferSize) {
        throw Error("Config(%s) - Can't read FIR file. '%s'", path.c_str(), file.getPath().c_str());
    }
    WaveHeader header;
    if (bufferSize < sizeof(header)) {
        throw Error("Config(%s) - FIR file is not a valid wav file. '%s'", path.c_str(), file.getPath().c_str());
    }
    memcpy(&header, pBuffer.get(), sizeof(header));
    const char* pData = pBuffer.get()  + sizeof(header);
    if (header.numChannels != 1) {
        throw Error("Config(%s) - FIR file is not mono", path.c_str());
    }
    if (header.sampleRate != _sampleRate) {
        throw Error("Config(%s) - FIR file sample rate doesn't match render device. Render(%d), FIR(%d)", path.c_str(), _sampleRate, header.sampleRate);
    }
    //Generate taps
    const size_t numSamples = header.getNumSamples();
    vector<double> taps;
    if (header.audioFormat == WAVE_FORMAT_PCM) {
        switch (header.bitsPerSample) {
        case 16:
            taps = Convert::pcm16ToDouble(pData, numSamples);
            break;
        case 24:
            taps = Convert::pcm24ToDouble(pData, numSamples);
            break;
        case 32:
            taps = Convert::pcm32ToDouble(pData, numSamples);
            break;
        default:
            throw Error("Config(%s) - FIR file bit depth is unsupported: %u", path.c_str(), header.bitsPerSample);
        }
    }
    else if (header.audioFormat == WAVE_FORMAT_IEEE_FLOAT) {
        switch (header.bitsPerSample) {
        case 32:
            taps = Convert::float32ToDouble(pData, numSamples);
            break;
        case 64:
            taps = Convert::float64ToDouble(pData, numSamples);
            break;
        default:
            throw Error("Config(%s) - FIR file bit depth is unsupported: %u", path.c_str(), header.bitsPerSample);
        }
    }
    else {
        throw Error("Config(%s) - FIR file is in unknown audio format: %u", path.c_str(), header.audioFormat);
    }
    filters.push_back(make_unique<FilterFir>(taps));
}

void Config::applyCrossoversMap(FilterBiquad* pFilterBiquad, const Channel channel, const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    //Check if this channel should have an LP and that there isn't an user defined LP in the filters list.
    if (_addLpTo.find(channel) != _addLpTo.end() && !hasCrossoverFilter(pFilterNode, true, path)) {
        parseCrossover(true, pFilterBiquad, _pLpFilter, "basic");
    }
    if (_addHpTo.find(channel) != _addHpTo.end() && !hasCrossoverFilter(pFilterNode, false, path)) {
        parseCrossover(false, pFilterBiquad, _pHpFilter, "basic");
    }
}

void Config::applyCrossoversMap(vector<unique_ptr<Filter>>& filters, const Channel channel) const {
    if (_addLpTo.find(channel) != _addLpTo.end()) {
        unique_ptr<FilterBiquad> pFilterBiquad = make_unique<FilterBiquad>(_sampleRate);
        parseCrossover(true, pFilterBiquad.get(), _pLpFilter, "basic");
        filters.push_back(move(pFilterBiquad));
    }
    if (_addHpTo.find(channel) != _addHpTo.end()) {
        unique_ptr<FilterBiquad> pFilterBiquad = make_unique<FilterBiquad>(_sampleRate);
        parseCrossover(false, pFilterBiquad.get(), _pHpFilter, "basic");
        filters.push_back(move(pFilterBiquad));
    }
}

const double Config::getQOffset(const shared_ptr<JsonNode>& pFilterNode, const string& path) const {
    if (pFilterNode->has("qOffset")) {
        return getDoubleValue(pFilterNode, "qOffset", path);
    }
    return 0;
}

const vector<double> Config::getQValues(const shared_ptr<JsonNode>& pFilterNode, const int order, const string& path) const {
    string qPath = path;
    const shared_ptr<JsonNode> pQNode = getArrayNode(pFilterNode, "q", qPath);
    vector<double> qValues;
    int calculatedOrder = 0;
    for (size_t i = 0; i < pQNode->size(); ++i) {
        const double q = getDoubleValue(pQNode, i, qPath);
        calculatedOrder += q < 0 ? 1 : 2;
        qValues.push_back(q);
    }
    if (calculatedOrder != order) {
        throw Error("Config(%s) - CROSSOVER.CUSTOM: Q values list doesn't match order. Expected(%d), Found(%d)", path.c_str(), order, calculatedOrder);
    }
    return qValues;
}

const bool Config::hasCrossoverFilter(const shared_ptr<JsonNode>& pFiltersNode, const bool isLowPass, const string& path) const {
    for (size_t i = 0; i < pFiltersNode->size(); ++i) {
        string filterPath = path;
        const shared_ptr<JsonNode> pFilter = getObjectNode(pFiltersNode, i, filterPath);
        const FilterType type = getFilterType(pFilter, "type", path);
        if ((isLowPass && type == FilterType::LOW_PASS) || (!isLowPass && type == FilterType::HIGH_PASS)) {
            return true;
        }
    }
    return false;
}