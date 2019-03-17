#include "Config.h"
#include "DSP.h"
#include "JsonNode.h"
#include "WinDSPLog.h"
#include "FilterType.h"
#include "SubType.h"
#include "CrossoverType.h"
#include "Str.h"
#include "Convert.h"
#include "AudioDevice.h" //WAVE_FORMAT_PCM, WAVE_FORMAT_IEEE_FLOAT
#include "Channel.h"

const std::vector<Filter*> Config::parseFilters(const JsonNode *pNode, const std::string &path, const int outputChannel) {
    std::vector<Filter*> filters;
    //Parse single instance simple filters.
    parseGain(filters, pNode, path);
    parseDelay(filters, pNode, path);
    //Parse filters list
    FilterBiquad *pFilterBiquad = new FilterBiquad(_sampleRate);
    std::string filtersPath = path;
    const JsonNode *pFiltersNode = tryGetArrayNode(pNode, "filters", filtersPath);
    for (size_t i = 0; i < pFiltersNode->size(); ++i) {
        std::string filterPath = filtersPath;
        const JsonNode *pFilter = getObjectNode(pFiltersNode, i, filterPath);
        parseFilter(filters, pFilterBiquad, pFilter, outputChannel, filterPath);
    }

    //Apply crossovers from basic config.
    applyCrossoversMap(pFilterBiquad, outputChannel);

    //No biquads added. Don't use biquad filter.
    if (pFilterBiquad->isEmpty()) {
        delete pFilterBiquad;
    }
    //Use  biquad filter.
    else {
        filters.push_back(pFilterBiquad);
    }
    return filters;
}

const std::vector<Filter*> Config::parsePostFilters(const JsonNode *pNode, const std::string &path) {
    std::vector<Filter*> filters;
    parseCancellation(filters, pNode, path);
    return filters;
}

void Config::parseCancellation(std::vector<Filter*> &filters, const JsonNode *pNode, const std::string &path) const {
    if (pNode->has("cancellation")) {
        const JsonNode *pFilterNode = pNode->path("cancellation");
        const double freq = getDoubleValue(pFilterNode, "freq", path);
        if (pFilterNode->has("gain")) {
            const double gain = getDoubleValue(pFilterNode, "gain", path);
            filters.push_back(new FilterCancellation(_sampleRate, freq, gain));
        }
        else {
            filters.push_back(new FilterCancellation(_sampleRate, freq));
        }
    }
}

void Config::parseGain(std::vector<Filter*> &filters, const JsonNode *pNode, const std::string &path) {
    const double value = tryGetDoubleValue(pNode, "gain", path);
    const bool invert = tryGetBoolValue(pNode, "invert", path);
    //No use in adding zero gain.
    if (value != 0.0 || invert) {
        filters.push_back(new FilterGain(value, invert));
    }
}

void Config::parseDelay(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) {
    pNode = tryGetNode(pNode, "delay", path);
    if (pNode->isMissingNode()) {
        return;
    }
    double value;
    double useUnitMeter = false;
    if (pNode->isNumber()) {
        value = pNode->doubleValue();
    }
    else if (pNode->isObject()) {
        value = getDoubleValue(pNode, "value", path);
        useUnitMeter = tryGetBoolValue(pNode, "unitMeter", path);
    }
    else {
        throw Error("Config(%s) - Invalid delay format. Expected number or object", path.c_str());
    }
    //No use in adding zero delay.
    if (value != 0) {
        const uint32_t sampleDelay = FilterDelay::getSampleDelay(_sampleRate, value, useUnitMeter);
        if (sampleDelay > 0) {
            filters.push_back(new FilterDelay(_sampleRate, value, useUnitMeter));
        }
        else {
            LOG_WARN("WARNING: Config(%s) - Discarding delay filter with to low value. Can't delay less then one sample", path.c_str());
        }
    }
}

void Config::parseFilter(std::vector<Filter*> &filters, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const int outputChannel, const std::string &path) {
    //Is array. Iterate and parse each filter.
    if (pFilterNode->isArray()) {
        for (size_t i = 0; i < pFilterNode->size(); ++i) {
            parseFilter(filters, pFilterBiquad, pFilterNode->get(i), outputChannel, path + "/" + std::to_string(i));
        }
        return;
    }
    std::string typeStr;
    const FilterType type = getFilterType(pFilterNode, "type", typeStr, path);
    switch (type) {
    case FilterType::LOW_PASS:
    case FilterType::HIGH_PASS: {
        const bool isLP = type == FilterType::LOW_PASS;
        parseCrossover(isLP, pFilterBiquad, pFilterNode, path);
        //Update crossovers map from basic config.
        updateCrossoverMaps(isLP, outputChannel);
        break;
    }
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
        throw Error("Config(%s) - Unknown filter type '%s'", path.c_str(), typeStr.c_str());
    };
}

void Config::parseCrossover(const bool isLowPass, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
    std::string subTypeStr;
    const SubType subType = getSubType(pFilterNode, "subType", subTypeStr, path);
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const int order = getIntValue(pFilterNode, "order", path);
    switch (subType) {
    case SubType::BUTTERWORTH:
        pFilterBiquad->addCrossover(isLowPass, freq, CrossoverType::Butterworth, order, getQOffset(pFilterNode, path));
        break;
    case SubType::LINKWITZ_RILEY:
        pFilterBiquad->addCrossover(isLowPass, freq, CrossoverType::Linkwitz_Riley, order, getQOffset(pFilterNode, path));
        break;
    case SubType::BESSEL:
        pFilterBiquad->addCrossover(isLowPass, freq, CrossoverType::Bessel, order, getQOffset(pFilterNode, path));
        break;
    case SubType::CUSTOM:
        pFilterBiquad->addCrossover(isLowPass, freq, getQValues(pFilterNode, order, path));
        break;
    default:
        throw Error("Config(%s) - Unknown crossover sub type %d", path.c_str(), subTypeStr.c_str());
    }
}

void Config::parseShelf(const bool isLowShelf, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
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

void Config::parsePEQ(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
    const double freq = getDoubleValue(pFilterNode, "freq", path);
    const double q = getDoubleValue(pFilterNode, "q", path);
    const double gain = getDoubleValue(pFilterNode, "gain", path);
    pFilterBiquad->addPEQ(freq, q, gain);
}

void Config::parseBandPass(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
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

void Config::parseNotch(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
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

void Config::parseLinkwitzTransform(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
    const double f0 = getDoubleValue(pFilterNode, "f0", path);
    const double q0 = getDoubleValue(pFilterNode, "q0", path);
    const double fp = getDoubleValue(pFilterNode, "fp", path);
    const double qp = getDoubleValue(pFilterNode, "qp", path);
    pFilterBiquad->addLinkwitzTransform(f0, q0, fp, qp);
}

void Config::parseBiquad(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const {
    std::string myPath = path;
    const JsonNode *pValues = getArrayNode(pFilterNode, "values", myPath);
    for (size_t i = 0; i < pValues->size(); ++i) {
        const JsonNode *pValueNode = pValues->get(i);
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

void Config::parseFir(std::vector<Filter*> &filters, const JsonNode *pFilterNode, const std::string &path) const {
    std::string myPath = path;
    //Read each line in fir parameter file
    const std::string filePath = getTextValue(pFilterNode, "file", myPath);
    const File file(filePath);
    const std::string extension = file.getExtension();
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

void Config::parseFirTxt(std::vector<Filter*> &filters, const File &file, const std::string &path) const {
    std::vector<std::string> lines;
    if (!file.getData(lines)) {
        throw Error("Config(%s) - Can't read FIR file '%s'", path.c_str(), file.getPath().c_str());
    }
    if (!lines.size()) {
        throw Error("Config(%s) - FIR file '%s' is empty", path.c_str(), file.getPath().c_str());
    }
    std::vector<double> taps;
    for (const std::string &str : lines) {
        taps.push_back(atof(str.c_str()));
    }
    filters.push_back(new FilterFir(taps));
}

void Config::parseFirWav(std::vector<Filter*> &filters, const File &file, const std::string &path) const {
    char *pBuffer = nullptr;
    try {
        //Get data
        const size_t bufferSize = file.getData(&pBuffer);
        WaveHeader header;
        memcpy(&header, pBuffer, sizeof(header));
        const char *pData = pBuffer + sizeof(header);
        if (header.numChannels != 1) {
            throw Error("Config(%s) - FIR file is not mono", path.c_str());
        }
        if (header.sampleRate != _sampleRate) {
            throw Error("Config(%s) - FIR file sample rate doesn't match render device. Render(%d), FIR(%d)", path.c_str(), _sampleRate, header.sampleRate);
        }
        //Generate taps
        const size_t numSamples = header.getNumSamples();
        std::vector<double> taps;
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
        delete[] pBuffer;
        filters.push_back(new FilterFir(taps));
    }
    catch (const Error &e) {
        delete[] pBuffer;
        throw e;
    }
}

const double Config::getQOffset(const JsonNode *pFilterNode, const std::string &path) const {
    if (pFilterNode->has("qOffset")) {
        return getDoubleValue(pFilterNode, "qOffset", path);
    }
    return 0;
}

const std::vector<double> Config::getQValues(const JsonNode *pFilterNode, const int order, const std::string &path) const {
    std::string qPath = path;
    const JsonNode *pQNode = getArrayNode(pFilterNode, "q", qPath);
    std::vector<double> qValues;
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

void Config::updateCrossoverMaps(const bool isLP, const int outputChannel) {
    if (outputChannel > -1) {
        const Channel channel = (Channel)outputChannel;
        if (isLP) {
            if (_addLpTo.find(channel) != _addLpTo.end()) {
                _addLpTo.erase(channel);
            }
        }
        else {
            if (_addHpTo.find(channel) != _addHpTo.end()) {
                _addHpTo.erase(channel);
            }
        }
    }
}

void Config::applyCrossoversMap(FilterBiquad *pFilterBiquad, const int outputChannel) {
    if (outputChannel > -1) {
        const Channel channel = (Channel)outputChannel;
        const std::string basicPath = "basic";
        if (_addLpTo.find(channel) != _addLpTo.end() && _addLpTo[channel]) {
            std::string lpPath = basicPath;
            parseCrossover(true, pFilterBiquad, _pLpFilter, lpPath);
        }
        if (_addHpTo.find(channel) != _addHpTo.end() && _addHpTo[channel]) {
            std::string hpPath = basicPath;
            parseCrossover(false, pFilterBiquad, _pLpFilter, hpPath);
        }
    }
}