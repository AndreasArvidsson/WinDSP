#include "Config.h"
#define NOMINMAX
#include <iostream> //cin
#include "JsonParser.h"
#include "Convert.h"
#include "WinDSPLog.h"
#include "AudioDevice.h"
#include "Input.h"
#include "Output.h"
#include "Visibility.h"
#include "FilterGain.h"
#include "Channel.h"

Config::Config(const std::string &path) {
    _configFile = path;
    _hide = _minimize = _useConditionalRouting = _startWithOS = _addAutoGain = false;
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
    parseRouting();
    parseOutputs();

    //TODO
    printConfig();
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

void Config::setDevices() {
    Visibility::show(true);
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

const size_t Config::getSelection(const size_t start, const size_t end, const size_t blacklist) const {
    char buf[10];
    int value;
    do {
        std::cin.getline(buf, 10);
        value = atoi(buf);
    } while (value < start || value > end || value == blacklist);
    return value;
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
        printf("%s\t", Channels::toString(pI->getChannel()).c_str());
        for (const Route *pR : pI->getRoutes()) {
            printRouteConfig(pR->getChannel(), pR->getFilters(), pR->getFilters().size(), pR->hasConditions());
        }
        printf("\n");
    }
    printf("\n");

    printf("***** Outputs *****\n");
    for (const Output *pO : _outputs) {
        printRouteConfig(pO->getChannel(), pO->getFilters(), pO->getFilters().size() + pO->getPostFilters().size());
        printf("\n");
    }
    printf("\n");
}

void Config::printRouteConfig(const Channel channel, const std::vector<Filter*> &filters, size_t numFilters, const bool hasConditions) const {
    const double level = getFiltersLevelSum(filters);
    const double gain = Convert::levelToDb(level);
    printf("%s", Channels::toString(channel).c_str());
    if (gain) {
        printf("(%.1fdb)", gain);
    }
    if (hasConditions) {
        printf("(if)");
    }
    if (hasGainFilter(filters)) {
        //Retract the gain filter.
        --numFilters; 
    }
    if (numFilters) {
        printf("(%zdF)", numFilters);
    }
    printf(" ");
}