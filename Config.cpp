#include "Config.h"
#include <iostream> //cin
#include "Error.h"
#include "FilterType.h"
#include "SubType.h"
#include "JsonParser.h"
#include "Convert.h"
#include "OS.h"

Config::Config() {}

Config::~Config() {
	for (AudioDevice *p : _devices) {
		delete p;
	}
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
}

void Config::init(const std::string path) {
	_configFile = path;
	load();
	parseMisc();
	parseDevices();
}

void Config::init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut) {
	_sampleRate = sampleRate;
	_numChannelsIn = numChannelsIn;
	_numChannelsOut = numChannelsOut;
	parseInputs();
	parseOutputs();
}

const std::vector<AudioDevice*>& Config::getDevices() const {
	return _devices;
}

const std::vector<Input*>& Config::getInputs() const {
	return _inputs;
}

const std::vector<Output*>& Config::getOutputs() const {
	return _outputs;
}

const bool Config::hasChanged() const {
	return _lastModified != _configFile.getLastModifiedTime();
}

const bool Config::hide() const {
	return _hide;
}

const bool Config::minimize() const {
	return _minimize;
}

void Config::parseMisc() {
	//Parse visibility options
	if (_pJsonNode->has("hide")) {
		_hide = boolValue(_pJsonNode, "hide", "");
	}
	else {
		_hide = false;
	}
	if (_pJsonNode->has("minimize")) {
		_minimize = boolValue(_pJsonNode, "minimize", "");
	}
	else {
		_minimize = false;
	}

	std::string path = "channels";
	JsonNode *pChannels = _pJsonNode->path(path);
	if (!pChannels->isMissingNode()) {
		if (!pChannels->isArray()) {
			throw Error("Config(%s) - Field is not an array/list", path.c_str());
		}
		for (size_t i = 0; i < pChannels->size(); ++i) {
			JsonNode *pChannelName = pChannels->get(i);
			if (!pChannelName->isText()) {
				throw Error("Config(%s/%d) - Value is not a text string", path.c_str(), i);
			}
			_channelNames.push_back(pChannelName->textValue());
		}
	}
	else {
		//Default channel order.
		_channelNames = { "L", "R", "C", "SW", "SBL", "SBR" ,"SL", "SR" };
	}
}

void Config::parseDevices() {
	JsonNode *pDvicesNode = _pJsonNode->path("devices");
	//Devices not set in config. Query user
	if (!pDvicesNode->has("capture") || !pDvicesNode->has("render")) {
		setDevices();
	}
	//Devices set in config.
	else {
		std::string captureId = textValue(pDvicesNode->get("capture"), "devices/capture");
		std::string renderId = textValue(pDvicesNode->get("render"), "devices/render");
		_devices.push_back(new AudioDevice(captureId));
		_devices.push_back(new AudioDevice(renderId));
	}
}

void Config::parseInputs() {
	//Create default in to out routing
	_inputs = std::vector<Input*>(_numChannelsIn);
	//Iterate inputs and set routes
	std::string path = "inputs";
	JsonNode *pInputs = _pJsonNode->path(path);
	for (std::string channelName : pInputs->getOrder()) {
		parseInput(pInputs, channelName, path);
	}
	//Add default in/out route to missing
	for (size_t i = 0; i < _numChannelsIn; ++i) {
		if (!_inputs[i]) {
			//Output exists. Route to output
			if (i < _numChannelsOut) {
				_inputs[i] = new Input(i);
			}
			//Output doesn't exists. Add default non-route input.
			else {
				_inputs[i] = new Input();
			}
		}
	}
}

void Config::parseInput(const JsonNode *pInputs, const std::string &channelName, std::string path) {
	size_t channelIn = getChannelIndex(channelName, path);
	if (channelIn >= (int)_numChannelsIn) {
		printf("WARNING: Config(%s) - Capture device doesn't have channel '%s'\n", path.c_str(), channelName.c_str());
		return;
	}
	const JsonNode *pChannelNode = getNode(pInputs, channelName, path);
	_inputs[channelIn] = new Input();
	JsonNode *pRoutes = pChannelNode->path("routes");
	path = path + "/" + "routes";
	for (size_t i = 0; i < pRoutes->size(); ++i) {
		parseRoute(_inputs[channelIn]->routes, pRoutes, i, path);
	}
}

void Config::parseRoute(std::vector<Route*> &routes, const JsonNode *pRoutes, const size_t index, std::string path) {
	const JsonNode *pRouteNode = getNode(pRoutes, index, path);
	//If route have no out channel it's the same thing as no route at all.
	if (pRouteNode->has("out")) {
		std::string outPath = path + "/out";
		std::string channelName = textValue(pRouteNode->get("out"), outPath);
		size_t channelOut = getChannelIndex(channelName, outPath);
		if (channelOut >= (int)_numChannelsOut) {
			printf("WARNING: Config(%s) - Render device doesn't have channel '%s'\n", outPath.c_str(), channelName.c_str());
			return;
		}
		Route *pRoute = new Route(channelOut);
		routes.push_back(pRoute);
		parseFilters(pRoute->filters, pRouteNode, path);
		parseConditions(pRoute, pRouteNode, path);
	}
}

void Config::parseConditions(Route *pRoute, const JsonNode *pRouteNode, std::string path) {
	if (pRouteNode->has("if")) {
		const JsonNode *pIfNode = getNode(pRouteNode, "if", path);
		if (pIfNode->has("silent")) {
			std::string channelName = textValue(pIfNode, "silent", path);
			size_t channel = getChannelIndex(channelName, path + "/silent");
			pRoute->conditions.push_back(Condition(ConditionType::SILENT, (int)channel));
		}
		else {
			printf("WARNING: Config(%s) - Unknown if codition", path.c_str());
		}
	}
}

void Config::parseOutputs() {
	_outputs = std::vector<Output*>(_numChannelsOut);
	//Iterate outputs and add filters
	std::string path = "outputs";
	JsonNode *pOutputs = _pJsonNode->path(path);
	for (std::string channelName : pOutputs->getOrder()) {
		parseOutput(pOutputs, channelName, path);
	}
	//Add default empty output to missing
	for (size_t i = 0; i < _outputs.size(); ++i) {
		if (!_outputs[i]) {
			_outputs[i] = new Output();
			_outputs[i]->add(new OutputFork());
		}
	}
	validateLevels(path);
}

void Config::parseOutput(const JsonNode *pOutputs, const std::string &channelName, std::string path) {
	size_t channel = getChannelIndex(channelName, path);
	if (channel >= (int)_numChannelsOut) {
		printf("WARNING: Config(%s) - Render device doesn't have channel '%s'\n", path.c_str(), channelName.c_str());
		return;
	}
	const JsonNode *pChannelNode = getNode(pOutputs, channelName, path);
	Output *pOutput = new Output;
	_outputs[channel] = pOutput;
	//Array parse each fork.
	if (pChannelNode->isArray()) {
		for (size_t i = 0; i < pChannelNode->size(); ++i) {
			std::string tmpPath = path;
			const JsonNode *pForkNode = getNode(pChannelNode, i, tmpPath);
			parseOutputFork(pOutput, pForkNode, tmpPath);
		}
		//Empty array. Add default empty fork
		if (pChannelNode->size() == 0) {
			pOutput->add(new OutputFork());
		}
	}
	//Object. Parse single fork
	else {
		parseOutputFork(pOutput, pChannelNode, path);
	}
}

void Config::parseOutputFork(Output *pOutput, const JsonNode *pForkNode, std::string path) {
	bool mute = pForkNode->path("mute")->boolValue();
	//Muted output is the same as no fork at all
	if (!mute) {
		OutputFork *pFork = new OutputFork();
		pOutput->add(pFork);
		parseFilters(pFork->filters, pForkNode, path);
	}
}

void Config::validateLevels(const std::string &path) const {
	std::vector<double> levels;
	//Init levels to silence/0
	for (size_t i = 0; i < _outputs.size(); ++i) {
		levels.push_back(0);
	}
	//Apply input/route levels
	for (const Input *pInput : _inputs) {
		for (const Route *pRoute : pInput->routes) {
			//Conditional routing is not always applied at the same time as other route. Eg if silent.
			if (pRoute->conditions.size() == 0) {
				levels[pRoute->out] += getFilterGainSum(pRoute->filters);
			}
		}
	}
	//Apply output gain
	for (size_t i = 0; i < _outputs.size(); ++i) {
		double level = 0;
		for (const OutputFork *pFork : _outputs[i]->getForks()) {
			level += getFilterGainSum(pFork->filters, levels[i]);
		}
		levels[i] = level;
	}
	//Eval output channel levels
	bool first = true;
	for (size_t i = 0; i < levels.size(); ++i) {
		//Level is above 0dBFS. CLIPPING CAN OCCURE!!!
		if (levels[i] > 1.0) {
			if (first) {
				printf("WARNING: Config(%s) - Sum of routed channel levels is above 0dBFS on output channel. CLIPPING CAN OCCUR!\n", path.c_str());
				first = false;
			}
			printf("\t%s: +%.2f dBFS\n", getChannelName(i, path).c_str(), Convert::levelToDb(levels[i]));
		}
	}
}

const double Config::getFilterGainSum(const std::vector<Filter*> &filters, double startLevel) const {
	for (Filter *pFilter : filters) {
		//If filter is gain: Apply gain
		if (typeid(*pFilter) == typeid(GainFilter)) {
			startLevel = pFilter->process(startLevel);
		}
	}
	return startLevel;
}

void Config::parseFilters(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) {
	//Parse single instance simple filters
	parseGain(filters, pNode, path);
	parseDelay(filters, pNode, path);
	parseInvertPolarity(filters, pNode, path);
	//Parse filters list
	BiquadFilter *pBiquadFilter = new BiquadFilter(_sampleRate);
	JsonNode *pFiltersNode = pNode->path("filters");
	for (size_t i = 0; i < pFiltersNode->size(); ++i) {
		std::string tmpPath = path + "/filters";
		const JsonNode *pFilter = getNode(pFiltersNode, i, tmpPath);
		parseFilter(filters, pBiquadFilter, pFilter, tmpPath);
	}
	//No biquads added. Don't use biquad filter.
	if (pBiquadFilter->isEmpty()) {
		delete pBiquadFilter;
	}
	//Use  biquad filter
	else {
		filters.push_back(pBiquadFilter);
	}
}

void Config::parseGain(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) {
	pNode = getNode(pNode, "gain", path);
	if (!pNode->isMissingNode()) {
		const double value = doubleValue(pNode, path);
		//No use in adding zero gain.
		if (value != 0) {
			filters.push_back(new GainFilter(value));
		}
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
		int sampleDelay = DelayFilter::getSampleDelay(_sampleRate, value, useUnitMeter);
		if (sampleDelay > 0) {
			filters.push_back(new DelayFilter(_sampleRate, value, useUnitMeter));
		}
		else {
			printf("WARNING: Config(%s) - Discarding delay filter with to low value. Can't delay less then one sample\n", path.c_str());
		}
	}
}

void Config::parseInvertPolarity(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) const {
	if (pNode->has("invert")) {
		if (boolValue(pNode, "invert", path)) {
			filters.push_back(new InvertPolarityFilter());
		}
	}
}

void Config::parseFilter(std::vector<Filter*> &filters, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	//Is array. Iterate and parse each filter.
	if (pFilterNode->isArray()) {
		for (size_t i = 0; i < pFilterNode->size(); ++i) {
			parseFilter(filters, pBiquadFilter, pFilterNode->get(i), path + "/" + std::to_string(i));
		}
		return;
	}
	const std::string typeStr = textValue(pFilterNode, "type", path);
	FilterType type = FilterTypes::fromString(typeStr);
	switch (type) {
	case FilterType::LOW_PASS:
	case FilterType::HIGH_PASS:
		parseCrossover(type == FilterType::LOW_PASS, pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::LOW_SHELF:
	case FilterType::HIGH_SHELF:
		parseShelf(type == LOW_SHELF, pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::PEQ:
		parsePEQ(pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::BAND_PASS:
		parseBandPass(pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::NOTCH:
		parseNotch(pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::LINKWITZ_TRANSFORM:
		parseLinkwitzTransform(pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::BIQUAD:
		parseBiquad(pBiquadFilter, pFilterNode, path);
		break;
	case FilterType::FIR:
		parseFir(filters, pFilterNode, path);
		break;
	default:
		throw Error("Config(%s) - Unknown filter type '%s'", path.c_str(), FilterTypes::toString(type));
	};
}

void  Config::parseCrossover(const bool isLowPass, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	SubType subType = SubTypes::fromString(getField(pFilterNode, "subType", path)->textValue());
	double freq = doubleValue(pFilterNode, "freq", path);
	int order = getField(pFilterNode, "order", path)->intValue();
	switch (subType) {
	case SubType::BUTTERWORTH:
		pBiquadFilter->addCrossover(isLowPass, freq, order, CrossoverType::Butterworth);
		break;
	case SubType::LINKWITZ_RILEY:
		pBiquadFilter->addCrossover(isLowPass, freq, order, CrossoverType::Linkwitz_Riley);
		break;
	case SubType::BESSEL:
		pBiquadFilter->addCrossover(isLowPass, freq, order, CrossoverType::Bessel);
		break;
	case SubType::CUSTOM: {
		const JsonNode *pQNode = getField(pFilterNode, "q", path);
		std::vector<double> qValues;
		int calculatedOrder = 0;
		for (size_t i = 0; i < pQNode->size(); ++i) {
			double q = pQNode->get(i)->doubleValue();
			calculatedOrder += q < 0 ? 1 : 2;
			qValues.push_back(q);
		}
		if (calculatedOrder != order) {
			throw Error("Config(%s) - CROSSOVER.CUSTOM: Q values list doesn't match order. Expected(%d), Found(%d)", path.c_str(), order, calculatedOrder);
		}
		pBiquadFilter->addCrossover(isLowPass, freq, order, qValues);
		break;
	}
	default:
		throw Error("Config(%s) - Unknown crossover sub type %d", path.c_str(), SubTypes::toString(subType));
	}
}

void Config::parseShelf(const bool isLowShelf, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	double freq = doubleValue(pFilterNode, "freq", path);
	double gain = doubleValue(pFilterNode, "gain", path);
	double slope = 1.0;
	if (pFilterNode->has("slope")) {
		double slope = doubleValue(pFilterNode, "slope", path);
	}
	pBiquadFilter->addShelf(isLowShelf, freq, gain, slope);
}

void Config::parsePEQ(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	double freq = doubleValue(pFilterNode, "freq", path);
	double q = doubleValue(pFilterNode, "q", path);
	double gain = doubleValue(pFilterNode, "gain", path);
	pBiquadFilter->addPEQ(freq, q, gain);
}

void Config::parseBandPass(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	double freq = doubleValue(pFilterNode, "freq", path);
	double bandwidth = doubleValue(pFilterNode, "bandwidth", path);
	if (pFilterNode->has("gain")) {
		double gain = doubleValue(pFilterNode, "gain", path);
		pBiquadFilter->addBandPass(freq, bandwidth, gain);
	}
	else {
		pBiquadFilter->addBandPass(freq, bandwidth);
	}
}

void Config::parseNotch(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	double freq = doubleValue(pFilterNode, "freq", path);
	double bandwidth = doubleValue(pFilterNode, "bandwidth", path);
	pBiquadFilter->addNotch(freq, bandwidth);
}

void Config::parseLinkwitzTransform(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	double f0 = doubleValue(pFilterNode, "f0", path);
	double q0 = doubleValue(pFilterNode, "q0", path);
	double fp = doubleValue(pFilterNode, "fp", path);
	double qp = doubleValue(pFilterNode, "qp", path);
	pBiquadFilter->addLinkwitzTransform(f0, q0, fp, qp);
}

void Config::parseBiquad(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const {
	const JsonNode *pValues = getField(pFilterNode, "file", path);
	for (size_t i = 0; i < pValues->size(); ++i) {
		const JsonNode *pValueNode = pValues->get(i);
		double b0 = doubleValue(pValueNode->path("b0"), path);
		double b1 = doubleValue(pValueNode->path("b1"), path);
		double b2 = doubleValue(pValueNode->path("b2"), path);
		double a1 = doubleValue(pValueNode->path("a1"), path);
		double a2 = doubleValue(pValueNode->path("a2"), path);
		pBiquadFilter->add(b0, b1, b2, a1, a2);
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
	filters.push_back(new FirFilter(taps));
}

void Config::parseFirWav(std::vector<Filter*> &filters, const File &file, std::string path) const {
	char *pBuffer;
	try {
		//Get data
		size_t bufferSize = file.getData(&pBuffer);
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
		filters.push_back(new FirFilter(taps));
	}
	catch (const Error &e) {
		delete[] pBuffer;
		throw e;
	}
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
		//pResult = getReference(pRefNode, path);
		pRefNode = getReference(pRefNode, path);
		pResult = getEnrichedReference(pRefNode, pResult);
	}
	return pResult;
}

const JsonNode* Config::getNode(const JsonNode *pNode, const size_t index, std::string &path) {
	const JsonNode *pResult = pNode->path(index);
	path = path + "/" + std::to_string(index);
	JsonNode *pRefNode = pResult->path("#ref");
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
	for (auto &pair : pRefNode->getFields()) {
		pCopy->put(pair.first, pair.second);
	}
	//Then copy org fields. IE org fields have presedence in a conflict.
	for (auto &pair : pOrgNode->getFields()) {
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

void Config::setDevices() {
	OS::showWindow();
	std::vector<AudioDevice*> allDevices = AudioDevice::getDevices();
	size_t captureIndex, renderIndex;
	bool isOk;
	do {
		printf("Available playback devices:\n");
		for (size_t i = 0; i < allDevices.size(); ++i) {
			printf("%zu %s\n", i, allDevices[i]->getName().c_str());
		}
		//Make selection
		printf("\nSelect capture device\n");
		captureIndex = getSelection(allDevices.size());
		printf("\nSelect render device(Can't be same as capture)\n");
		renderIndex = getSelection(allDevices.size(), captureIndex);
		//Query if selection is ok.
		printf("\nSelected devices:\n");
		printf("Capture: %s\n", allDevices[captureIndex]->getName().c_str());
		printf("Render: %s\n", allDevices[renderIndex]->getName().c_str());
		printf("Press 1 to continue or 0 to re-select\n");
		isOk = getSelection(2) == 1;
		printf("\n");
	} while (!isOk);

	//Update json
	JsonNode *pDevicesNode = new JsonNode(JsonNodeType::OBJECT);
	pDevicesNode->put("capture", allDevices[captureIndex]->getId());
	pDevicesNode->put("render", allDevices[renderIndex]->getId());
	_pJsonNode->put("devices", pDevicesNode);
	save();

	//Store selected devices
	_devices.push_back(allDevices[captureIndex]);
	_devices.push_back(allDevices[renderIndex]);

	//Delete non used devices
	for (size_t i = 0; i < allDevices.size(); ++i) {
		if (i != captureIndex && i != renderIndex) {
			delete allDevices[i];
		}
	}
}

const size_t Config::getSelection(const size_t size, const size_t blacklist) const {
	char c;
	do {
		c = std::cin.get();
	} while (c < '0' || c >= '0' + size || c == '0' + blacklist);
	return c - '0';
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