#include "Config.h"
#include "WinDSPLog.h"
#include "JsonNode.h"
#include "Input.h"
#include "Output.h"
#include "Channel.h"

void Config::parseDevices() {
    const JsonNode *pDevicesNode = _pJsonNode->path("devices");
    //Devices not set in config. Query user
    if (!pDevicesNode->has("capture") || !pDevicesNode->has("render")) {
        setDevices();
        parseDevices();
    }
    //Devices already set in config.
    else {
        _captureDeviceName = getTextValue(pDevicesNode, "capture", "devices");
        _renderDeviceName = getTextValue(pDevicesNode, "render", "devices");
    }
}

void Config::parseMisc() {
    //Parse visibility options
    _hide = tryGetBoolValue(_pJsonNode, "hide", "");
    _minimize = tryGetBoolValue(_pJsonNode, "minimize", "");
    //Parse startup
    _startWithOS = tryGetBoolValue(_pJsonNode, "startWithOS", "");
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
    //Add default empty output to missing
    for (size_t i = 0; i < _outputs.size(); ++i) {
        if (!_outputs[i]) {
            _outputs[i] = new Output((Channel)i);
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

const std::vector<Channel> Config::getOutputChannels(const JsonNode *pOutputNode, const std::string &path) {
    std::vector<Channel> result;
    std::string channelsPath = path;
    const JsonNode *pChannels = getArrayNode(pOutputNode, "channels", channelsPath);
    for (size_t i = 0; i < pChannels->size(); ++i) {
        const std::string channelName = getTextValue(pChannels, i, channelsPath);
        const Channel channel = Channels::fromString(channelName);
        const size_t channelIndex = (size_t)channel;
        if (channelIndex >= _numChannelsOut) {
            LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", channelsPath.c_str(), channelName.c_str());
            continue;
        }
        if (_outputs[channelIndex]) {
            throw Error("Config(%s/%d) - Channel '%s' is already defiend/used", channelsPath.c_str(), i, Channels::toString(channel).c_str());
        }
        result.push_back(channel);
    }
    return result;
}