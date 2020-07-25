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
#include "AsioDevice.h"

using std::make_shared;
using std::make_unique;
using std::move;
using std::to_string;

#define NOMINMAX
#include <iostream> //cin

void Config::parseDevices() {
    string path;
    const shared_ptr<JsonNode> pDevicesNode = tryGetObjectNode(_pJsonNode, "devices", path);
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
    _inputs = vector<Input>(_numChannelsIn);

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
}

void Config::parseOutputs() {
    _outputs = vector<Output>(_numChannelsOut);
    //Iterate outputs and add filters
    string path = "";
    const shared_ptr<JsonNode> pOutputs = tryGetArrayNode(_pJsonNode, "outputs", path);
    for (size_t i = 0; i < pOutputs->size(); ++i) {
        parseOutput(pOutputs, i, path);
    }
    //Add default output to missing(not defined by user in conf) output channels.
    for (size_t i = 0; i < _outputs.size(); ++i) {
        if (!_outputs[i].isDefined()) {
            const Channel channel = (Channel)i;
            Output output(channel);
            vector<unique_ptr<Filter>> filters;
            //Check if there is basic routing crossovers to apply.
            applyCrossoversMap(filters, channel);
            output.addFilters(filters);
            _outputs[i] = move(output);
        }
    }
    validateLevels(path);
}

void Config::parseOutput(const shared_ptr<JsonNode>& pOutputs, const size_t index, string path) {
    const shared_ptr<JsonNode> pOutputNode = getObjectNode(pOutputs, index, path);
    const vector<Channel> channels = getOutputChannels(pOutputNode, path);
    for (const Channel channel : channels) {
        const bool mute = tryGetBoolValue(pOutputNode, "mute", path);
        Output output(channel, mute);
        vector<unique_ptr<Filter>> filters = parseFilters(pOutputNode, path, (int)channel);
        parseCancellation(filters, pOutputNode, path);
        output.addFilters(filters);
        _outputs[(size_t)channel] = move(output);
    }
}

const vector<Channel> Config::getOutputChannels(const shared_ptr<JsonNode>& pOutputNode, const string& path) const {
    vector<Channel> result;
    if (pOutputNode->has("channels")) {
        string channelsPath = path;
        const shared_ptr<JsonNode> pChannels = getArrayNode(pOutputNode, "channels", channelsPath);
        for (size_t i = 0; i < pChannels->size(); ++i) {
            const Channel channel = getOutputChannel(
                getTextValue(pChannels, i, channelsPath),
                channelsPath + "/" + to_string(i)
            );

            if (channel != Channel::CHANNEL_NULL) {
                result.push_back(channel);
            }
        }
    }
    else if (pOutputNode->has("channel")) {
        const Channel channel = getOutputChannel(
            getTextValue(pOutputNode, "channel", path), 
            path + "/channel"
        );
        if (channel != Channel::CHANNEL_NULL) {
            result.push_back(channel);
        }
    }
    else {
        throw Error("Config(%s) - 'channels' array/List or 'channel' text string is required", path.c_str());
    }
    return result;
}

const Channel Config::getOutputChannel(const string& channelName, const string& path) const {
    const Channel channel = Channels::fromString(channelName);
    const size_t channelIndex = (size_t)channel;
    if (channelIndex >= _numChannelsOut) {
        LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", path.c_str(), channelName.c_str());
        return Channel::CHANNEL_NULL;
    }
    if (_outputs[channelIndex].isDefined()) {
        throw Error("Config(%s) - Channel '%s' is already defiend/used", path.c_str(), Channels::toString(channel).c_str());
    }
    return channel;
}

void Config::validateLevels(const string& path) {
    vector<double> levels(_outputs.size());
    //Apply input/route levels
    for (const Input& input : _inputs) {
        for (const Route& route : input.getRoutes()) {
            //Conditional routing is not always applied at the same time as other route. Eg if silent.
            if (!route.hasConditions()) {
                levels[route.getChannelIndex()] += getFiltersLevelSum(route.getFilters());
            }
        }
    }
    //Apply output gain
    for (size_t i = 0; i < _outputs.size(); ++i) {
        levels[i] = getFiltersLevelSum(_outputs[i].getFilters(), levels[i]);
    }
    //Eval output channel levels
    bool first = true;
    for (size_t i = 0; i < _outputs.size(); ++i) {
        //Level is above 0dBFS. CLIPPING CAN OCCURE!!!
        if (levels[i] > 1) {
            const double gain = Convert::levelToDb(levels[i]);

            //Add enough gain to avoid clipping.
            if (_addAutoGain) {
                Output& output = _outputs[i];
                FilterGain* pFilterGain = getGainFilter(output.getFilters());
                //Add 0.1 to be sure to cover range
                const double newGain = -(gain + 0.1);
                if (pFilterGain) {
                    //Existing filter with new gain. Must be invert only. Set new gain.
                    if (!pFilterGain->getGain()) {
                        pFilterGain->setGain(newGain);
                        continue;
                    }
                }
                //Gain filter missing. Add new one.
                else {
                    output.addFilterFirst(make_unique<FilterGain>(newGain));
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
    const vector<string> wasapiDevices = AudioDevice::getDeviceNames();
    const vector<string> asioDevices = AsioDevice::getDeviceNames();

    size_t selectedIndex;
    bool isOk, renderAsio;
    do {
        LOG_INFO("Available capture devices:");
        LOG_NL();
        for (size_t i = 0; i < wasapiDevices.size(); ++i) {
            LOG_INFO("%zu - %s - WASAPI", i + 1, wasapiDevices[i].c_str());
        }
        LOG_NL();
        LOG_INFO("Press corresponding number to select");
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
                LOG_INFO("%zu - %s - WASAPI", index, wasapiDevices[i].c_str());
            }
        }
        for (size_t i = 0; i < asioDevices.size(); ++i, ++index) {
            LOG_INFO("%zu - %s - ASIO\n", index, asioDevices[i].c_str());
        }
        LOG_NL();
        LOG_INFO("Press corresponding number to select");
        //Make selection
        selectedIndex = getSelection(1, wasapiDevices.size() + asioDevices.size(), selectedIndex);
        renderAsio = selectedIndex - 1 >= wasapiDevices.size();

        //Query if selection is ok.
        LOG_NL();
        LOG_INFO("Selected devices:");
        LOG_NL();
        LOG_INFO("Capture - %s - WASAPI", _captureDeviceName.c_str());
        if (renderAsio) {
            _renderDeviceName = asioDevices[selectedIndex - wasapiDevices.size() - 1];
            LOG_INFO("Render  - %s - ASIO\n", _renderDeviceName.c_str());
        }
        else {
            _renderDeviceName = wasapiDevices[selectedIndex - 1];
            LOG_INFO("Render  - %s - WASAPI\n", _renderDeviceName.c_str());
        }
        LOG_NL();
        LOG_INFO("Press 1 to continue or 0 to re-select");
        isOk = getSelection(0, 1) == 1;
        LOG_NL();
    } while (!isOk);

    //Update json

    if (!_pJsonNode->path("devices")->isObject()) {
        _pJsonNode->put("devices", make_shared<JsonNode>(JsonNodeType::OBJECT));
    }
    shared_ptr<JsonNode>pDevicesNode = _pJsonNode->get("devices");

    pDevicesNode->put("capture", _captureDeviceName);
    pDevicesNode->put("render", _renderDeviceName);
    pDevicesNode->put("renderAsio", renderAsio);

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