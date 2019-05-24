#include "Config.h"
#include "JsonParser.h"
#include "Input.h"
#include "Output.h"
#include "FilterGain.h"

Config::Config(const std::string &path) {
    _configFile = path;
    _hide = _minimize = _useConditionalRouting = _startWithOS = _addAutoGain = _debug = false;
    _sampleRate = _numChannelsIn = _numChannelsOut = 0;
    _lastModified = 0;
    _pLpFilter = _pHpFilter = nullptr;
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
    delete _pJsonNode;
    _pJsonNode = nullptr;
    _pLpFilter = _pHpFilter = nullptr;
}

void Config::init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut) {
    _sampleRate = sampleRate;
    _numChannelsIn = numChannelsIn;
    _numChannelsOut = numChannelsOut;
    _useConditionalRouting = false;
    _debug = tryGetBoolValue(_pJsonNode, "debug", "");
    parseRouting();
    parseOutputs();

    if (_debug) {
        printConfig();
    }
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

const double Config::getFiltersLevelSum(const std::vector<Filter*> &filters, double startLevel) const {
    for (const Filter * const pFilter : filters) {
        //If filter is gain: Apply gain
        if (typeid (*pFilter) == typeid (FilterGain)) {
            const FilterGain *pFilterGain = (FilterGain*)pFilter;
            startLevel *= pFilterGain->getMultiplierNoInvert();
        }
    }
    return startLevel;
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
    return getTextValue(_pJsonNode, "description", "");
}

const bool Config::hasDescription() const {
    return _pJsonNode->has("description");
}

const bool Config::inDebug() const {
    return _debug;
}

const bool Config::useAsioRenderDevice() const {
    //TODO
    return true;
}

const bool Config::hasGainFilter(const std::vector<Filter*> &filters) const {
    for (const Filter *pFilter : filters) {
        if (typeid (*pFilter) == typeid (FilterGain)) {
            return true;
        }
    }
    return false;
}

void Config::printConfig() const {
    printf("***** Inputs *****\n");
    for (const Input *pI : _inputs) {
        printf("%s", Channels::toString(pI->getChannel()).c_str());
        for (const Route *pR : pI->getRoutes()) {
            printf("\t");
            printRouteConfig(pR->getChannel(), pR->getFilters(), {}, pR->hasConditions());
        }
    }
    printf("\n");

    printf("***** Outputs *****\n");
    for (const Output *pO : _outputs) {
        printRouteConfig(pO->getChannel(), pO->getFilters(), pO->getPostFilters());
    }
    printf("\n");
}

void Config::printRouteConfig(const Channel channel, const std::vector<Filter*> &filters, const std::vector<Filter*> &postFilters, const bool hasConditions) const {
    printf("%s", Channels::toString(channel).c_str());
    if (hasConditions) {
        printf("\t[ Cond ]");
    }
    if (filters.size() + postFilters.size()) {
        printf("\t");
        for (const Filter *pFilter : filters) {
            printf("[ %s ] ", pFilter->toString().c_str());
        }
        for (const Filter *pFilter : postFilters) {
            printf("[ %s ] ", pFilter->toString().c_str());
        }
    }
    printf("\n");
}