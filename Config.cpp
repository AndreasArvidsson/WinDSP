#include "Config.h"
#include <iostream> //cin
#include <algorithm> //std::min
#include <locale>
#include "Error.h"
#include "FilterType.h"
#include "SubType.h"
#include "JsonNode.h"
#include "JsonParser.h"
#include "Convert.h"
#include "OS.h"
#include "WinDSPLog.h"
#include "AudioDevice.h"
#include "Input.h"
#include "Output.h"
#include "DSP.h"
#include "TrayIcon.h"
#include "Str.h"

Config::Config(const std::string &path) {
	_configFile = path;
	_hide = _minimize = _useConditionalRouting = _startWithOS = false;
	_sampleRate = _numChannelsIn = _numChannelsOut = _numChannelsRender = 0;
	_lastModified = 0;
	load();
	parseMisc();
	parseDevices();
}

Config::~Config() {
	for (Input *p : _inputs) {
		delete p;
	}
	for (Output *p : _outputs) {
		delete p;
	}
	for (JsonNode *p : _tmpJsonNodes) {
		//tmp list contains shallow copies. Need to remove children first othervise they will be deleted twice and cause errors.
		p->clear();
		delete p;
	}
	delete _pJsonNode;
	_pJsonNode = nullptr;
}

void Config::init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut) {
	_sampleRate = sampleRate;
	_numChannelsIn = numChannelsIn;
	_numChannelsOut = numChannelsOut;
	_useConditionalRouting = false;
	parseInputs();
	parseOutputs();
}

const std::string Config::getCaptureDeviceName() const {
	return _captureDeviceName;
}

const std::string Config::getRenderDeviceName() const {
	return _renderDeviceName;
}

const std::vector<Input*>* Config::getInputs() const {
	return &_inputs;
}

const std::vector<Output*>* Config::getOutputs() const {
	return &_outputs;
}

const bool Config::startWithOS() const {
	return _startWithOS;
}

const bool Config::hide() const {
	return _hide;
}

const bool Config::minimize() const {
	return _minimize;
}

void Config::parseMisc() {
	//Parse visibility options
	_hide = _pJsonNode->has("hide") ? boolValue(_pJsonNode, "hide", "") : false;
	_minimize = _pJsonNode->has("minimize") ? boolValue(_pJsonNode, "minimize", "") : false;

	//Parse startup
	_startWithOS = _pJsonNode->has("startWithOS") ? boolValue(_pJsonNode, "startWithOS", "") : false;

	//Parse channels
    const std::string path = "channels";
	const JsonNode *pChannels = _pJsonNode->path(path);
	if (!pChannels->isMissingNode()) {
		if (!pChannels->isArray()) {
			throw Error("Config(%s) - Field is not an array/list", path.c_str());
		}
		for (size_t i = 0; i < pChannels->size(); ++i) {
            const std::string channelName = textValue(pChannels->get(i), String::format("%s/%d", path.c_str(), i));
            _channelNames.push_back(channelName);
		}
	}
	else {
		//Default channel order.
		_channelNames = { "L", "R", "C", "SW", "SBL", "SBR", "SL", "SR" };
	}
}

void Config::parseDevices() {
    const JsonNode *pDvicesNode = _pJsonNode->path("devices");
    const JsonNode *pCaptureNode = pDvicesNode->path("capture");
	const JsonNode *pRenderNode = pDvicesNode->path("render");

	//Devices not set in config. Query user
	if (!pCaptureNode->has("name") || !pRenderNode->has("name")) {
		setDevices();
		parseDevices();
	}
	//Devices already set in config.
	else {
		_captureDeviceName = textValue(pCaptureNode, "name", "devices/capture");
		_renderDeviceName = textValue(pRenderNode, "name", "devices/render");
		_numChannelsRender = 0;
	}
}

void Config::parseInputs() {
	//Create default in to out routing
	_inputs = std::vector<Input*>(_numChannelsIn);
	//Iterate inputs and set routes
    const std::string path = "inputs";
    const JsonNode *pInputs = _pJsonNode->path(path);
	for (std::string channelName : pInputs->getOrder()) {
		parseInput(pInputs, channelName, path);
	}
	//Add default in/out route to missing
	for (size_t i = 0; i < _numChannelsIn; ++i) {
		if (!_inputs[i]) {
			//Output exists. Route to output
			if (i < _numChannelsOut) {
				_inputs[i] = new Input(getChannelName(i), i);
			}
			//Output doesn't exists. Add default non-route input.
			else {
				_inputs[i] = new Input(getChannelName(i));
			}
		}
	}
}

void Config::parseInput(const JsonNode *pInputs, const std::string &channelName, std::string path) {
    const size_t channelIn = getChannelIndex(channelName, path);
	if (channelIn >= (int)_numChannelsIn) {
		LOG_WARN("WARNING: Config(%s) - Capture device doesn't have channel '%s'\n", path.c_str(), channelName.c_str());
		return;
	}
	const JsonNode *pChannelNode = getNode(pInputs, channelName, path);
	_inputs[channelIn] = new Input(channelName);
    const JsonNode *pRoutes = pChannelNode->path("routes");
	path = path + "/" + "routes";
	for (size_t i = 0; i < pRoutes->size(); ++i) {
		parseRoute(_inputs[channelIn], pRoutes, i, path);
	}
}

void Config::parseRoute(Input *pInput, const JsonNode *pRoutes, const size_t index, std::string path) {
	const JsonNode *pRouteNode = getNode(pRoutes, index, path);
	//If route have no out channel it's the same thing as no route at all.
	if (pRouteNode->has("out")) {
        const std::string outPath = path + "/out";
        const std::string channelName = textValue(pRouteNode->get("out"), outPath);
		const size_t channelOut = getChannelIndex(channelName, outPath);
		if (channelOut >= (int)_numChannelsOut) {
			LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'\n", outPath.c_str(), channelName.c_str());
			return;
		}
		Route *pRoute = new Route(channelOut);
		pRoute->addFilters(parseFilters(pRouteNode, path));
		parseConditions(pRoute, pRouteNode, path);
		pInput->addRoute(pRoute);
	}
}

void Config::parseConditions(Route *pRoute, const JsonNode *pRouteNode, std::string path) {
	if (pRouteNode->has("if")) {
		const JsonNode *pIfNode = getNode(pRouteNode, "if", path);
		if (pIfNode->has("silent")) {
            const std::string channelName = textValue(pIfNode, "silent", path);
            const size_t channel = getChannelIndex(channelName, path + "/silent");
			pRoute->addCondition(Condition(ConditionType::SILENT, (int)channel));
		}
		else {
			LOG_WARN("WARNING: Config(%s) - Unknown if condition", path.c_str());
			return;
		}
		_useConditionalRouting = true;
	}
}

void Config::parseOutputs() {
	_outputs = std::vector<Output*>(_numChannelsOut);
	//Iterate outputs and add filters
    const std::string path = "outputs";
    const JsonNode *pOutputs = _pJsonNode->path(path);
	for (const std::string &channelName : pOutputs->getOrder()) {
		parseOutput(pOutputs, channelName, path);
	}
	//Add default empty output to missing
	for (size_t i = 0; i < _outputs.size(); ++i) {
		if (!_outputs[i]) {
			_outputs[i] = new Output(getChannelName(i));
		}
	}
	validateLevels(path);
}

void Config::parseOutput(const JsonNode *pOutputs, const std::string &channelName, std::string path) {
    const size_t channel = getChannelIndex(channelName, path);
	if (channel >= (int)_numChannelsOut) {
		LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'\n", path.c_str(), channelName.c_str());
		return;
	}
	const JsonNode *pChannelNode = getNode(pOutputs, channelName, path);
    bool mute = false;
    if (pChannelNode->has("mute")) {
        mute = boolValue(pChannelNode, "mute", path);
    }
	Output *pOutput = new Output(channelName, mute);
	pOutput->addFilters(parseFilters(pChannelNode, path));
	pOutput->addPostFilters(parsePostFilters(pChannelNode, path));
	_outputs[channel] = pOutput;
}

void Config::validateLevels(const std::string &path) const {
	std::vector<double> levels;
	//Init levels to silence/0
	for (size_t i = 0; i < _outputs.size(); ++i) {
		levels.push_back(0);
	}
	//Apply input/route levels
	for (const Input *pInput : _inputs) {
		for (const Route * const pRoute : pInput->getRoutes()) {
			//Conditional routing is not always applied at the same time as other route. Eg if silent.
			if (!pRoute->hasConditions()) {
				levels[pRoute->getChannelIndex()] += getFilterGainSum(pRoute->getFilters());
			}
		}
	}
	//Apply output gain
	for (size_t i = 0; i < _outputs.size(); ++i) {
		levels[i] = getFilterGainSum(_outputs[i]->getFilters(), levels[i]);
	}
	//Eval output channel levels
	bool first = true;
	for (size_t i = 0; i < levels.size(); ++i) {
		//Level is above 0dBFS. CLIPPING CAN OCCURE!!!
		if (levels[i] > 1.0) {
			if (first) {
				LOG_WARN("WARNING: Config(%s) - Sum of routed channel levels is above 0dBFS on output channel. CLIPPING CAN OCCUR!\n", path.c_str());
				first = false;
			}
			LOG_WARN("\t%s: +%.2f dBFS\n", getChannelName(i, path).c_str(), Convert::levelToDb(levels[i]));
		}
	}
	if (!first) {
		LOG_NL();
	}
}

const double Config::getFilterGainSum(const std::vector<Filter*> &filters, double startLevel) const {
	for (const Filter * const pFilter : filters) {
		//If filter is gain: Apply gain
		if (typeid (*pFilter) == typeid (FilterGain)) {
			const FilterGain *pFilterGain = (FilterGain*)pFilter;
			startLevel *= pFilterGain->getMultiplierNoInvert();
		}
	}
	return startLevel;
}

const std::vector<Filter*> Config::parseFilters(const JsonNode *pNode, std::string path) {
	std::vector<Filter*> filters;
	//Parse single instance simple filters
	parseGain(filters, pNode, path);
	parseDelay(filters, pNode, path);
	//Parse filters list
	FilterBiquad *pFilterBiquad = new FilterBiquad(_sampleRate);
    const JsonNode *pFiltersNode = pNode->path("filters");
	for (size_t i = 0; i < pFiltersNode->size(); ++i) {
		std::string tmpPath = path + "/filters";
		const JsonNode *pFilter = getNode(pFiltersNode, i, tmpPath);
		parseFilter(filters, pFilterBiquad, pFilter, tmpPath);
	}
	//No biquads added. Don't use biquad filter.
	if (pFilterBiquad->isEmpty()) {
		delete pFilterBiquad;
	}
	//Use  biquad filter
	else {
		filters.push_back(pFilterBiquad);
	}
	return filters;
}

const std::vector<Filter*> Config::parsePostFilters(const JsonNode *pNode, std::string path) {
	std::vector<Filter*> filters;
	parseCancellation(filters, pNode, path);
	return filters;
}

void Config::parseCancellation(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) const {
	if (pNode->has("cancellation")) {
		const JsonNode *pFilterNode = pNode->path("cancellation");
		const double freq = doubleValue(pFilterNode, "freq", path); 
		if (pFilterNode->has("gain")) {
			const double gain = doubleValue(pFilterNode, "gain", path);
			filters.push_back(new FilterCancellation(_sampleRate, freq, gain));
		}
		else {
			filters.push_back(new FilterCancellation(_sampleRate, freq));
		}
	}
}

void Config::parseGain(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) {
	double value = 0.0;
	bool invert = false;
	if (pNode->has("gain")) {
		value = doubleValue(pNode, "gain", path);
	}
	if (pNode->has("invert")) {
		invert = boolValue(pNode, "invert", path);
	}
	//No use in adding zero gain.
	if (value != 0.0 || invert) {
		filters.push_back(new FilterGain(value, invert));
	}
}

void Config::parseDelay(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) {
	pNode = getNode(pNode, "delay", path);
	if (pNode->isMissingNode()) {
		return;
	}
	double value;
	double useUnitMeter = false;
	if (pNode->isNumber()) {
		value = doubleValue(pNode, path);
	}
	else if (pNode->isObject()) {
		value = doubleValue(pNode, "value", path);
		if (pNode->has("unitMeter")) {
			useUnitMeter = boolValue(pNode->path("unitMeter"), path + "/unitMeter");
		}
	}
	else {
		throw Error("Config(%s) - Invalid delay format", path.c_str());
	}
	//No use in adding zero delay.
	if (value != 0) {
		const int sampleDelay = FilterDelay::getSampleDelay(_sampleRate, value, useUnitMeter);
		if (sampleDelay > 0) {
			filters.push_back(new FilterDelay(sampleDelay));
		}
		else {
			LOG_WARN("WARNING: Config(%s) - Discarding delay filter with to low value. Can't delay less then one sample\n", path.c_str());
		}
	}
}

void Config::parseFilter(std::vector<Filter*> &filters, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	//Is array. Iterate and parse each filter.
	if (pFilterNode->isArray()) {
		for (size_t i = 0; i < pFilterNode->size(); ++i) {
			parseFilter(filters, pFilterBiquad, pFilterNode->get(i), path + "/" + std::to_string(i));
		}
		return;
	}
	const std::string typeStr = textValue(pFilterNode, "type", path);
	const FilterType type = FilterTypes::fromString(typeStr);
	switch (type) {
	case FilterType::LOW_PASS:
	case FilterType::HIGH_PASS:
		parseCrossover(type == FilterType::LOW_PASS, pFilterBiquad, pFilterNode, path);
		break;
	case FilterType::LOW_SHELF:
	case FilterType::HIGH_SHELF:
		parseShelf(type == LOW_SHELF, pFilterBiquad, pFilterNode, path);
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
		throw Error("Config(%s) - Unknown filter type '%s'", path.c_str(), FilterTypes::toString(type));
	};
}

void Config::parseCrossover(const bool isLowPass, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
    const std::string subTypeStr = textValue(pFilterNode, "subType", path);
	const SubType subType = SubTypes::fromString(subTypeStr);
	const double freq = doubleValue(pFilterNode, "freq", path);
    const int order = intValue(pFilterNode, "order", path);
	switch (subType) {
	case SubType::BUTTERWORTH:
		pFilterBiquad->addCrossover(isLowPass, freq, order, CrossoverType::Butterworth);
		break;
	case SubType::LINKWITZ_RILEY:
		pFilterBiquad->addCrossover(isLowPass, freq, order, CrossoverType::Linkwitz_Riley);
		break;
	case SubType::BESSEL:
		pFilterBiquad->addCrossover(isLowPass, freq, order, CrossoverType::Bessel);
		break;
	case SubType::CUSTOM: {
		const JsonNode *pQNode = getField(pFilterNode, "q", path);
		std::vector<double> qValues;
		int calculatedOrder = 0;
		for (size_t i = 0; i < pQNode->size(); ++i) {
			const double q = doubleValue(pQNode->get(i), String::format("%s/%d", path.c_str(), i));
			calculatedOrder += q < 0 ? 1 : 2;
			qValues.push_back(q);
		}
		if (calculatedOrder != order) {
			throw Error("Config(%s) - CROSSOVER.CUSTOM: Q values list doesn't match order. Expected(%d), Found(%d)", path.c_str(), order, calculatedOrder);
		}
		pFilterBiquad->addCrossover(isLowPass, freq, qValues);
		break;
	}
	default:
		throw Error("Config(%s) - Unknown crossover sub type %d", path.c_str(), SubTypes::toString(subType));
	}
}

void Config::parseShelf(const bool isLowShelf, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const double freq = doubleValue(pFilterNode, "freq", path);
	const double gain = doubleValue(pFilterNode, "gain", path);
	if (pFilterNode->has("q")) {
		const double q = doubleValue(pFilterNode, "q", path);
		pFilterBiquad->addShelf(isLowShelf, freq, gain, q);
	}
	else {
		pFilterBiquad->addShelf(isLowShelf, freq, gain);
	}
}

void Config::parsePEQ(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const double freq = doubleValue(pFilterNode, "freq", path);
	const double q = doubleValue(pFilterNode, "q", path);
	const double gain = doubleValue(pFilterNode, "gain", path);
	pFilterBiquad->addPEQ(freq, q, gain);
}

void Config::parseBandPass(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const double freq = doubleValue(pFilterNode, "freq", path);
	const double bandwidth = doubleValue(pFilterNode, "bandwidth", path);
	if (pFilterNode->has("gain")) {
		const double gain = doubleValue(pFilterNode, "gain", path);
		pFilterBiquad->addBandPass(freq, bandwidth, gain);
	}
	else {
		pFilterBiquad->addBandPass(freq, bandwidth);
	}
}

void Config::parseNotch(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const double freq = doubleValue(pFilterNode, "freq", path);
	const double bandwidth = doubleValue(pFilterNode, "bandwidth", path);
	if (pFilterNode->has("gain")) {
		const double gain = doubleValue(pFilterNode, "gain", path);
		pFilterBiquad->addNotch(freq, bandwidth, gain);
	}
	else {
		pFilterBiquad->addNotch(freq, bandwidth);
	}
}

void Config::parseLinkwitzTransform(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const double f0 = doubleValue(pFilterNode, "f0", path);
	const double q0 = doubleValue(pFilterNode, "q0", path);
	const double fp = doubleValue(pFilterNode, "fp", path);
	const double qp = doubleValue(pFilterNode, "qp", path);
	pFilterBiquad->addLinkwitzTransform(f0, q0, fp, qp);
}

void Config::parseBiquad(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string path) const {
	const JsonNode *pValues = getField(pFilterNode, "values", path);
	for (size_t i = 0; i < pValues->size(); ++i) {
		const JsonNode *pValueNode = pValues->get(i);
		const double b0 = doubleValue(pValueNode->path("b0"), path);
		const double b1 = doubleValue(pValueNode->path("b1"), path);
		const double b2 = doubleValue(pValueNode->path("b2"), path);
		const double a1 = doubleValue(pValueNode->path("a1"), path);
		const double a2 = doubleValue(pValueNode->path("a2"), path);
		if (pValueNode->has("a0")) {
			const double a0 = doubleValue(pValueNode->path("a0"), path);
			pFilterBiquad->add(b0, b1, b2, a0, a1, a2);
		}
		else {
			pFilterBiquad->add(b0, b1, b2, a1, a2);
		}
	}
}

void Config::parseFir(std::vector<Filter*> &filters, const JsonNode *pFilterNode, std::string path) const {
	//Read each line in fir parameter file
	File file(textValue(pFilterNode, "file", path));
	path = path + "/file";
	const std::string extension = file.getExtension();
	if (extension.compare("txt") == 0) {
		parseFirTxt(filters, file, path);
	}
	else if (extension.compare("wav") == 0) {
		parseFirWav(filters, file, path);
	}
	else {
		throw Error("Config(%s) - Unknown file extension for FIR file '%s'", path.c_str(), file.getPath().c_str());
	}
}

void Config::parseFirTxt(std::vector<Filter*> &filters, const File &file, std::string path) const {
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

void Config::parseFirWav(std::vector<Filter*> &filters, const File &file, std::string path) const {
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

void Config::setDevices() {
	OS::showWindow();
	TrayIcon::hide();
	const std::vector<std::string> wasapiDevices = AudioDevice::getDeviceNames();

	size_t selectedIndex;
	bool isOk;
	do {
		LOG_INFO("Available capture devices:\n");
		for (size_t i = 0; i < wasapiDevices.size(); ++i) {
			LOG_INFO("%zu: %s\n", i + 1, wasapiDevices[i].c_str());
		}
		//Make selection
		selectedIndex = getSelection(1, wasapiDevices.size());
		_captureDeviceName = wasapiDevices[selectedIndex - 1];

		LOG_INFO("\nAvailable render devices:\n");
		size_t index = 1;
		for (size_t i = 0; i < wasapiDevices.size(); ++i, ++index) {
			//Cant reuse capture device.
			if (index != selectedIndex) {
				LOG_INFO("%zu: %s\n", index, wasapiDevices[i].c_str());
			}
		}
		//Make selection
		selectedIndex = getSelection(1, wasapiDevices.size(), selectedIndex);

		//Query if selection is ok.
		LOG_INFO("\nSelected devices:\n");
		LOG_INFO("Capture: %s\n", _captureDeviceName.c_str());
		_renderDeviceName = wasapiDevices[selectedIndex - 1];
		LOG_INFO("Render: %s\n", _renderDeviceName.c_str());
		LOG_INFO("\nPress 1 to continue or 0 to re-select\n");
		isOk = getSelection(0, 1) == 1;
		LOG_NL();
	} while (!isOk);

	//Update json

	if (!_pJsonNode->path("devices")->isObject()) {
		_pJsonNode->put("devices", new JsonNode(JsonNodeType::OBJECT));
	}
	JsonNode *pDevicesNode = _pJsonNode->get("devices");

	if (!_pJsonNode->path("capture")->isObject()) {
		pDevicesNode->put("capture", new JsonNode(JsonNodeType::OBJECT));
	}
	JsonNode *pCaptureNode = pDevicesNode->get("capture");

	if (!_pJsonNode->path("render")->isObject()) {
		pDevicesNode->put("render", new JsonNode(JsonNodeType::OBJECT));
	}
	JsonNode *pRenderNode = pDevicesNode->get("render");

	pCaptureNode->put("name", _captureDeviceName);
	pRenderNode->put("name", _renderDeviceName);
	save();
}

const JsonNode* Config::getField(const JsonNode *pNode, const std::string &field, const std::string &path) const {
	const JsonNode *pResult = pNode->path(field);
	if (pResult->isMissingNode()) {
		throw Error("Config(%s) - Field '%s' is required", path.c_str(), field.c_str());
	}
	return pResult;
}

const JsonNode* Config::getNode(const JsonNode *pNode, const std::string &field, std::string &path) {
	const JsonNode *pResult = pNode->path(field);
	path = path + "/" + field;
	const JsonNode *pRefNode = pResult->path("#ref");
	if (!pRefNode->isMissingNode()) {
		pRefNode = getReference(pRefNode, path);
		pResult = getEnrichedReference(pRefNode, pResult);
	}
	return pResult;
}

const JsonNode* Config::getNode(const JsonNode *pNode, const size_t index, std::string &path) {
	const JsonNode *pResult = pNode->path(index);
	path = path + "/" + std::to_string(index);
	const JsonNode *pRefNode = pResult->path("#ref");
	if (!pRefNode->isMissingNode()) {
		pResult = getReference(pRefNode, path);
	}
	return pResult;
}

const JsonNode* Config::getEnrichedReference(const JsonNode *pRefNode, const JsonNode *pOrgNode) {
	//The "#ref" field is the only one present. No need to create enriched copy.
	if (pOrgNode->size() == 1) {
		return pRefNode;
	}
	JsonNode *pCopy = new JsonNode(JsonNodeType::OBJECT);
	//Store in tmp list so destructor can take care of them.
	_tmpJsonNodes.push_back(pCopy);
	//First copy ref fields.
	for (const auto &pair : pRefNode->getFields()) {
		pCopy->put(pair.first, pair.second);
	}
	//Then copy org fields. IE org fields have presedence in a conflict.
	for (const auto &pair : pOrgNode->getFields()) {
		if (pair.first.compare("#ref") != 0) {
			pCopy->put(pair.first, pair.second);
		}
	}
	return pCopy;
}

const JsonNode* Config::getReference(const JsonNode *pRefNode, std::string &path) const {
	path = path + "/#ref";
	const std::string refPath = textValue(pRefNode, path);
	JsonNode *pNode = _pJsonNode;
	char buf[500];
	strcpy(buf, refPath.c_str());
	char *part = strtok(buf, "/");
	while (part != NULL) {
		pNode = pNode->path(part);
		part = strtok(NULL, "/");
	}
	if (pNode->isMissingNode()) {
		throw Error("Config(%s) - Can't find reference '%s'", path.c_str(), refPath.c_str());
	}
	path = path + " -> " + refPath;
	return pNode;
}

const double Config::doubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
	const JsonNode *pResult = getField(pNode, field, path);
	return doubleValue(pResult, path + "/" + field);
}

const double Config::doubleValue(const JsonNode *pNode, const std::string &path) const {
	if (!pNode->isNumber()) {
		throw Error("Config(%s) - Field is not a number", path.c_str());
	}
	return pNode->doubleValue();
}

const int Config::intValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
	const JsonNode *pResult = getField(pNode, field, path);
	return intValue(pResult, path + "/" + field);
}

const int Config::intValue(const JsonNode *pNode, const std::string &path) const {
	if (!pNode->isInteger()) {
		throw Error("Config(%s) - Field is not an integer number", path.c_str());
	}
	return pNode->intValue();
}

const std::string Config::textValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
	const JsonNode *pResult = getField(pNode, field, path);
	return textValue(pResult, path + "/" + field);
}

const std::string Config::textValue(const JsonNode *pNode, const std::string &path) const {
	if (!pNode->isText()) {
		throw Error("Config(%s) - Field is not a text string", path.c_str());
	}
	return pNode->textValue();
}

const bool Config::boolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
	const JsonNode *pResult = getField(pNode, field, path);
	return boolValue(pResult, path + "/" + field);
}

const bool Config::boolValue(const JsonNode *pNode, const std::string &path) const {
	if (!pNode->isBoolean()) {
		throw Error("Config(%s) - Field is not a boolean", path.c_str());
	}
	return pNode->boolValue();
}

const size_t Config::getSelection(const size_t start, const size_t end, const size_t blacklist) const {
	char buf[10];
	int value;
	do {
		std::cin.getline(buf, 10);
		value = atoi(buf);
	} while (value < start || value > end || value == blacklist);
	return value;
}

const size_t Config::getChannelIndex(const std::string &channelName, const std::string &path) const {
	for (size_t i = 0; i < _channelNames.size(); ++i) {
		if (_channelNames[i].compare(channelName) == 0) {
			return i;
		}
	}
	throw Error("Config(%s) - Unknown channel '%s'", path.c_str(), channelName.c_str());
}

const std::string Config::getChannelName(const size_t channelIndex, const std::string &path) const {
	if (channelIndex >= _channelNames.size()) {
		throw Error("Config(%s) - Unknown channel index: %d", path.c_str(), channelIndex);
	}
	return _channelNames[channelIndex];
}

const uint32_t Config::getNumChannelsRender(const uint32_t capacity) const {
	if (_numChannelsRender > 0) {
		return _numChannelsRender;
	}
	return (uint32_t)std::min((uint32_t)_channelNames.size(), capacity);
}

const std::string Config::getChannelName(const size_t channelIndex) const {
	if (channelIndex >= _channelNames.size()) {
		throw Error("Unknown channel index: %d", channelIndex);
	}
	return _channelNames[channelIndex];
}

void Config::load() {
	_pJsonNode = JsonParser::fromFile(_configFile);
	_lastModified = _configFile.getLastModifiedTime();
}

void Config::save() {
	JsonParser::toFile(_configFile, *_pJsonNode);
	_lastModified = _configFile.getLastModifiedTime();
}

const std::string Config::getDescription() const {
	return textValue(_pJsonNode, "description", "");
}

const bool Config::hasDescription() const {
	return _pJsonNode->has("description");
}