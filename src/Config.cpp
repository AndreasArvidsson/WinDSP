#include "Config.h"
#include "JsonParser.h"
#include "Input.h"
#include "Output.h"
#include "FilterGain.h"
#include "WinDSPLog.h"

Config::Config(const string& path) {
    _configFile = path;
    _hide = _minimize = _useConditionalRouting = _startWithOS = _addAutoGain = _debug = _useAsioRenderDevice = false;
    _sampleRate = _numChannelsIn = _numChannelsOut = _asioBufferSize = _asioNumChannels = 0;
    _lastModified = 0;
    load();
    parseMisc();
    parseDevices();
}

void Config::init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut) {
    _sampleRate = sampleRate;
    _numChannelsIn = numChannelsIn;
    _numChannelsOut = numChannelsOut;
    _useConditionalRouting = false;
    parseRouting();
    parseOutputs();
}

const string Config::getCaptureDeviceName() const {
    return _captureDeviceName;
}

const string Config::getRenderDeviceName() const {
    return _renderDeviceName;
}

vector<Input>& Config::getInputs() {
    return _inputs;
}

vector<Output>& Config::getOutputs() {
    return _outputs;
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

const double Config::getFiltersLevelSum(const vector<unique_ptr<Filter>>& filters, double startLevel) const {
    for (const unique_ptr<Filter>& pFilter : filters) {
        //If filter is gain: Apply gain
        if (typeid (*pFilter.get()) == typeid (FilterGain)) {
            const FilterGain* pFilterGain = (FilterGain*)pFilter.get();
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

const string Config::getDescription() const {
    return getTextValue(_pJsonNode, "description", "");
}

const bool Config::hasDescription() const {
    return _pJsonNode->has("description");
}

const bool Config::inDebug() const {
    return _debug;
}

const bool Config::useAsioRenderDevice() const {
    return _useAsioRenderDevice;
}

const uint32_t Config::getAsioBufferSize() const {
    return _asioBufferSize;
}

const uint32_t Config::getAsioNumChannels() const {
    return _asioNumChannels;
}

const bool Config::useConditionalRouting() const {
    return _useConditionalRouting;
}

const bool Config::hasChanged() const {
    return _lastModified != _configFile.getLastModifiedTime();
}

FilterGain* Config::getGainFilter(const vector<unique_ptr<Filter>>& filters) {
    for (const unique_ptr<Filter>& pFilter : filters) {
        if (typeid (*pFilter) == typeid (FilterGain)) {
            return (FilterGain*)pFilter.get();
        }
    }
    return nullptr;
}

void Config::printConfig() const {
    LOG_INFO("***** Inputs *****");
    LOG_NL();
    for (const Input& input : _inputs) {
        for (const Route& route : input.getRoutes()) {
            LOG_INFO("%s\t->\t%s%s",
                Channels::toString(input.getChannel()).c_str(),
                Channels::toString(route.getChannel()).c_str(),
                (route.hasConditions() ? " ( Condition )" : "")
            );
            printFilters("", route.getFilters());
            LOG_NL();
        }
    }
    LOG_NL();

    LOG_INFO("***** Outputs *****");
    LOG_NL();
    for (const Output& output : _outputs) {
        if (output.isMuted()) {
            LOG_INFO("%s\tMUTE", Channels::toString(output.getChannel()).c_str());
        }
        else {
            LOG_INFO(Channels::toString(output.getChannel()).c_str());
            printFilters("", output.getFilters());
        }
        LOG_NL();
    }
    LOG_NL();
}

void Config::printFilters(const string& prefix, const vector<unique_ptr<Filter>>& filters) const {
    for (const unique_ptr<Filter>& pFilter : filters) {
        for (const string& s : pFilter->toString()) {
            LOG_INFO("%s* %s", prefix.c_str(), s.c_str());
        }
    }
}