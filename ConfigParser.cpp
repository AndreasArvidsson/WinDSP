#include "Config.h"
#include "WinDSPLog.h"
#include "JsonNode.h"
#include "Input.h"
#include "Output.h"
#include "Channel.h"
#include "Convert.h"
#include "FilterGain.h"
#include "Visibility.h"
#include "AudioDevice.h"

#define NOMINMAX
#include <iostream> //cin

void Config::parseDevices() {
    std::string path;
    const JsonNode *pDevicesNode = tryGetObjectNode(_pJsonNode, "devices", path);
    //Devices not set in config. Query user
    if (!pDevicesNode->has("capture") || !pDevicesNode->has("render")) {
        setDevices();
        parseDevices();
    }
    //Devices already set in config.
    else {
        path = "devices";
        _captureDeviceName = getTextValue(pDevicesNode, "capture", path);
        _renderDeviceName = getTextValue(pDevicesNode, "render", path);
        _useAsioRenderDevice = tryGetBoolValue(pDevicesNode, "renderAsio", path);
        _asioBufferSize = tryGetIntValue(pDevicesNode, "asioBufferSize", path);
        _asioNumChannels = tryGetIntValue(pDevicesNode, "asioNumChannels", path);
    }
}

void Config::parseMisc() {
    //Parse visibility options
    _hide = tryGetBoolValue(_pJsonNode, "hide", "");
    _minimize = tryGetBoolValue(_pJsonNode, "minimize", "");
    //Parse startup
    _startWithOS = tryGetBoolValue(_pJsonNode, "startWithOS", "");
    //Parse debug
    _debug = tryGetBoolValue(_pJsonNode, "debug", "");
    //Log to file in debug mode.
    WinDSPLog::setLogToFile(_debug);
}

void Config::parseRouting() {
    //Create list of in to out routings
    _inputs = std::vector<Input*>(_numChannelsIn);

    //Use basic or advanced routing
    const bool hasBasic = _pJsonNode->has("basic");
    const bool hasAdvanced = _pJsonNode->has("advanced");
    if (hasBasic && hasAdvanced) {
        throw Error("Config(/) - Found both basic and advanced routing. Use one!");
    }
    if (hasBasic) {
        parseBasic();
    }
    else if (hasAdvanced) {
        parseAdvanced();
    }
   
    //Add default in/out route to missing
    for (size_t i = 0; i < _numChannelsIn; ++i) {
        if (!_inputs[i]) {
            //Output exists. Route to output
            if (i < _numChannelsOut) {
                _inputs[i] = new Input((Channel)i, (Channel)i);
            }
            //Output doesn't exists. Add default non-route input.
            else {
                _inputs[i] = new Input((Channel)i);
            }
        }
    }  
}

void Config::parseOutputs() {
    _outputs = std::vector<Output*>(_numChannelsOut);
    //Iterate outputs and add filters
    std::string path = "";
    const JsonNode *pOutputs = tryGetArrayNode(_pJsonNode, "outputs", path);
    for (size_t i = 0; i < pOutputs->size(); ++i) {
        parseOutput(pOutputs, i, path);
    }
    //Add default output to missing(not defined by user in conf) output channels.
    for (size_t i = 0; i < _outputs.size(); ++i) {
        if (!_outputs[i]) {
            const Channel channel = (Channel)i;
            _outputs[i] = new Output(channel);
            std::vector<Filter*> filters;
            //Check if there is basic routing crossovers to apply.
            applyCrossoversMap(filters, channel);
            if (filters.size()) {
                _outputs[i]->addFilters(filters);
            }
        }
    }
    validateLevels(path);
}

void Config::parseOutput(const JsonNode *pOutputs, const size_t index, std::string path) {
    const JsonNode *pOutputNode = getObjectNode(pOutputs, index, path);
    const std::vector<Channel> channels = getOutputChannels(pOutputNode, path);
    for (const Channel channel : channels) {
        const bool mute = tryGetBoolValue(pOutputNode, "mute", path);
        Output *pOutput = new Output(channel, mute);
        pOutput->addFilters(parseFilters(pOutputNode, path, (int)channel));
        pOutput->addPostFilters(parsePostFilters(pOutputNode, path));
        _outputs[(size_t)channel] = pOutput;
    }
}

const std::vector<Channel> Config::getOutputChannels(const JsonNode *pOutputNode, const std::string &path) const {
    std::vector<Channel> result;
    if (pOutputNode->has("channels")) {
        std::string channelsPath = path;
        const JsonNode *pChannels = getArrayNode(pOutputNode, "channels", channelsPath);
        for (size_t i = 0; i < pChannels->size(); ++i) {
            Channel channel;
            if (getOutputChannel(pChannels->get(i), channel, channelsPath + "/" + std::to_string(i))) {
                result.push_back(channel);
            }
        }
    }
    else if (pOutputNode->has("channel")) {
        Channel channel;
        if (getOutputChannel(pOutputNode->get("channel"), channel, path + "/channel")) {
            result.push_back(channel);
        }
    }
    else {
        throw Error("Config(%s) - 'channels' array/List or 'channel' text string is required", path.c_str());
    }
    return result;
}

const bool Config::getOutputChannel(const JsonNode *pChannelNode, Channel &channelOut, const std::string &path) const {
    const std::string channelName = validateTextValue(pChannelNode, path, false);
    const Channel channel = Channels::fromString(channelName);
    const size_t channelIndex = (size_t)channel;
    if (channelIndex >= _numChannelsOut) {
        LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", path.c_str(), channelName.c_str());
        return false;
    }
    if (_outputs[channelIndex]) {
        throw Error("Config(%s) - Channel '%s' is already defiend/used", path.c_str(), Channels::toString(channel).c_str());
    }
    channelOut = channel;
    return true;
}

void Config::validateLevels(const std::string &path) const {
    std::vector<double> levels(_outputs.size());
    //Apply input/route levels
    for (const Input *pInput : _inputs) {
        for (const Route * const pRoute : pInput->getRoutes()) {
            //Conditional routing is not always applied at the same time as other route. Eg if silent.
            if (!pRoute->hasConditions()) {
                levels[pRoute->getChannelIndex()] += getFiltersLevelSum(pRoute->getFilters());
            }
        }
    }
    //Apply output gain
    for (size_t i = 0; i < _outputs.size(); ++i) {
        levels[i] = getFiltersLevelSum(_outputs[i]->getFilters(), levels[i]);
    }
    //Eval output channel levels
    bool first = true;
    for (size_t i = 0; i < _outputs.size(); ++i) {
        //Level is above 0dBFS. CLIPPING CAN OCCURE!!!
        if (levels[i] > 1) {
            const double gain = Convert::levelToDb(levels[i]);

            //Add enough gain to avoid clipping.
            if (_addAutoGain && gain > 0) {
                Output *pOutput = _outputs[i];
                //Don't add additional gain filter.
                if (!hasGainFilter(pOutput->getFilters())) {
                    //Add 0.1 to be sure to cover range
                    pOutput->addFilter(new FilterGain(-(gain + 0.1)));
                    continue;
                }
            }

            if (first) {
                LOG_INFO("WARNING: Config(%s) - Sum of routed channel levels is above 0dBFS on output channel. CLIPPING CAN OCCUR!", path.c_str());
                first = false;
            }
            LOG_INFO("\t%s: +%.2f dBFS", Channels::toString(i).c_str(), gain);
        }
    }
    if (!first) {
        LOG_NL();
    }
}

void Config::setDevices() {
    Visibility::show(true);
    const std::vector<std::string> wasapiDevices = AudioDevice::getDeviceNames();

    size_t selectedIndex;
    bool isOk;
    do {
        LOG_INFO("Available capture devices:");
        LOG_NL();
        for (size_t i = 0; i < wasapiDevices.size(); ++i) {
            LOG_INFO("%zu: %s", i + 1, wasapiDevices[i].c_str());
        }
        //Make selection
        selectedIndex = getSelection(1, wasapiDevices.size());
        _captureDeviceName = wasapiDevices[selectedIndex - 1];

        LOG_NL();
        LOG_INFO("Available render devices:");
        LOG_NL();
        size_t index = 1;
        for (size_t i = 0; i < wasapiDevices.size(); ++i, ++index) {
            //Cant reuse capture device.
            if (index != selectedIndex) {
                LOG_INFO("%zu: %s", index, wasapiDevices[i].c_str());
            }
        }
        //Make selection
        selectedIndex = getSelection(1, wasapiDevices.size(), selectedIndex);

        //Query if selection is ok.
        LOG_NL();
        LOG_INFO("Selected devices:");
        LOG_NL();
        LOG_INFO("Capture: %s", _captureDeviceName.c_str());
        _renderDeviceName = wasapiDevices[selectedIndex - 1];
        LOG_INFO("Render: %s", _renderDeviceName.c_str());
        LOG_NL();
        LOG_INFO("Press 1 to continue or 0 to re-select");
        isOk = getSelection(0, 1) == 1;
        LOG_NL();
    } while (!isOk);

    //Update json

    if (!_pJsonNode->path("devices")->isObject()) {
        _pJsonNode->put("devices", new JsonNode(JsonNodeType::OBJECT));
    }
    JsonNode *pDevicesNode = _pJsonNode->get("devices");

    pDevicesNode->put("capture", _captureDeviceName);
    pDevicesNode->put("render", _renderDeviceName);

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