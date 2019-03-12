#include "Config.h"
#include "SpeakerType.h"
#include "JsonNode.h"
#include <algorithm> //std::find
#include "WinDSPLog.h"
#include "Channel.h"
#include "Input.h"
#include "FilterGain.h"
#include "FilterBiquad.h"

#define PHANTOM_CENTER_GAIN             -3 //Gain for routing center to L/R
#define LFE_GAIN                        10 //Gain for LFE channel to match other channels
#define BASS_TO_STEREO_GAIN             -6 //Gain for routing single bass channel to two

void Config::parseBasic() {
    _addAutoGain = true;
    std::string path = "";
    JsonNode *pBasicNode = (JsonNode*)getObjectNode(_pJsonNode, "basic", path);
    //Get channels
    const double stereoBass = tryGetBoolValue(pBasicNode, "stereoBass", path);
    std::vector<Channel> subs, subLs, subRs, smalls;
    const std::unordered_map<Channel, SpeakerType> channelsMap = parseChannels(pBasicNode, stereoBass, subs, subLs, subRs, smalls, path);
    const bool useSubwoofers = getUseSubwoofers(subs, subLs, subRs);
    const double lfeGain = getLfeGain(pBasicNode, useSubwoofers, stereoBass, smalls.size(), path);

    //Parse crossover config
    parseCrossover(pBasicNode, channelsMap, path);
    //Route input to output channels
    routeChannels(channelsMap, stereoBass, subs, subLs, subRs, lfeGain);
    //Add conditional routing to surrounds
    parseExpandSurround(pBasicNode, channelsMap, path);
}

void Config::parseCrossover(JsonNode *pBasicNode, const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::string &path) {
    for (const auto &e : channelsMap) {
        switch (e.second) {
        case SpeakerType::SMALL:
            _addHpTo[e.first] = true;
            break;
        case SpeakerType::SUB:
            _addLpTo[e.first] = true;
            break;
        }
    }

    JsonNode *pCrossoverNode;
    if (pBasicNode->has("lowPass")) {
        std::string myPath = path;
        pCrossoverNode = (JsonNode*)tryGetObjectNode(pBasicNode, "lowPass", myPath);
        if (pCrossoverNode->has("type") && !pCrossoverNode->has("subType")) {
            pCrossoverNode->renameField("type", "subType");
        }
    }
    else {
        pCrossoverNode = new JsonNode(JsonNodeType::OBJECT);
        pBasicNode->put("lowPass", pCrossoverNode);
    }
    addNonExisting(pCrossoverNode, "subType", "BUTTERWORTH");
    addNonExisting(pCrossoverNode, "freq", 80);
    addNonExisting(pCrossoverNode, "order", 5);
    _pLpFilter = pCrossoverNode;

    if (pBasicNode->has("highPass")) {
        std::string myPath = path;
        pCrossoverNode = (JsonNode*)tryGetObjectNode(pBasicNode, "highPass", myPath);
        if (pCrossoverNode->has("type") && !pCrossoverNode->has("subType")) {
            pCrossoverNode->renameField("type", "subType");
        }
    }
    else {
        pCrossoverNode = new JsonNode(JsonNodeType::OBJECT);
        pBasicNode->put("highPass", pCrossoverNode);
    }
    addNonExisting(pCrossoverNode, "subType", "BUTTERWORTH");
    addNonExisting(pCrossoverNode, "freq", 80);
    addNonExisting(pCrossoverNode, "order", 3);
    _pHpFilter = pCrossoverNode;
}

void Config::parseExpandSurround(const JsonNode *pBasicNode, const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::string &path) {
    const bool expandSurround = tryGetBoolValue(pBasicNode, "expandSurround", path);
    if (expandSurround) {
        const SpeakerType surrType = channelsMap.at(Channel::SL);
        const SpeakerType surrBackType = channelsMap.at(Channel::SBL);
        //All 4 surround channels must be playing as speakers
        if (surrType != SpeakerType::LARGE && surrType != SpeakerType::SMALL ||
            surrBackType != SpeakerType::LARGE && surrBackType != SpeakerType::SMALL) {
            LOG_WARN("WARNING: Config(%s/expandSurround) - Can't expand surround if not both surround and surround back are playing speakers as large/small", path.c_str());
            return;
        }
        addIfRoute(_inputs[(size_t)Channel::SL], Channel::SBL);
        addIfRoute(_inputs[(size_t)Channel::SR], Channel::SBR);
        _useConditionalRouting = true;
    }
}

void Config::routeChannels(const std::unordered_map<Channel, SpeakerType> &channelsMap, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) {
    for (size_t i = 0; i < _numChannelsIn; ++i) {
        const Channel channel = (Channel)i;
        Input *pInput = new Input(channel);
        _inputs[i] = pInput;
        const SpeakerType type = channelsMap.at(channel);
        switch (type) {
            //Route to matching speaker
        case SpeakerType::LARGE:
            addRoute(pInput, channel);
            break;
            //Route to matching speaker and sub/front
        case SpeakerType::SMALL:
            addRoute(pInput, channel);
            addBassRoute(pInput, stereoBass, subs, subLs, subRs, lfeGain);
            break;
            //Downmix
        case SpeakerType::OFF:
            downmix(channelsMap, pInput, stereoBass, subs, subLs, subRs, lfeGain);
            break;
            //Sub or downmix
        case SpeakerType::SUB:
            if (channel == Channel::SW) {
                addSwRoute(pInput, stereoBass, subs, subLs, subRs, lfeGain);
            }
            else {
                downmix(channelsMap, pInput, stereoBass, subs, subLs, subRs, lfeGain);
            }
            break;
        default:
            LOG_WARN("Unmanaged type '%s'", SpeakerTypes::toString(type).c_str());
        }
    }
}

void Config::downmix(const std::unordered_map<Channel, SpeakerType> &channelsMap, Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const {
    SpeakerType type = SpeakerType::OFF;
    switch (pInput->getChannel()) {
    case Channel::SL:
        type = addRoute(channelsMap, pInput, { Channel::SBL , Channel::L });
        break;
    case Channel::SBL:
        type = addRoute(channelsMap, pInput, { Channel::SL , Channel::L });
        break;
    case Channel::SR:
        type = addRoute(channelsMap, pInput, { Channel::SBR , Channel::R });
        break;
    case Channel::SBR:
        type = addRoute(channelsMap, pInput, { Channel::SR , Channel::R });
        break;
    case Channel::C:
        addRoute(pInput, Channel::L, PHANTOM_CENTER_GAIN);
        addRoute(pInput, Channel::R, PHANTOM_CENTER_GAIN);
        type = channelsMap.at(Channel::L);
        break;
    case Channel::SW:
        type = SpeakerType::SMALL;
        break;
    default:
        LOG_WARN("Unmanaged downmix channel '%s'", Channels::toString(pInput->getChannel()).c_str());
    }
    //Downmixed a small speaker. Add to subs as well.
    if (type == SpeakerType::SMALL) {
        addBassRoute(pInput, stereoBass, subs, subLs, subRs, lfeGain);
    }
}

void Config::addBassRoute(Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const {
    const Channel channel = pInput->getChannel();
    const double gain = (channel == Channel::SW ? lfeGain : 0);
    if (getUseSubwoofers(subs, subLs, subRs)) {
        addSwRoute(pInput, stereoBass, subs, subLs, subRs, gain);
    }
    else {
        addFrontBassRoute(pInput, stereoBass, gain);
    }
}

void Config::addFrontBassRoute(Input *pInput, const bool stereoBass, const double gain) const {
    const Channel channel = pInput->getChannel();
    const bool addLP = (channel != Channel::SW);
    if (stereoBass) {
        switch (channel) {
            //Route to left front channel
        case Channel::SL:
        case Channel::SBL:
            addRoute(pInput, Channel::L, gain, addLP);
            break;
            //Route to right front channel
        case Channel::SR:
        case Channel::SBR:
            addRoute(pInput, Channel::R, gain, addLP);
            break;
            //Route to both front channels
        case Channel::C:
        case Channel::SW:
            addRoute(pInput, Channel::L, gain + BASS_TO_STEREO_GAIN, addLP);
            addRoute(pInput, Channel::R, gain + BASS_TO_STEREO_GAIN, addLP);
        }
    }
    //Route to both front channels
    else {
        addRoute(pInput, Channel::L, gain + BASS_TO_STEREO_GAIN, addLP);
        addRoute(pInput, Channel::R, gain + BASS_TO_STEREO_GAIN, addLP);
    }
}

void Config::addSwRoute(Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double gain) const {
    if (stereoBass) {
        switch (pInput->getChannel()) {
            //Route to left sub channels
        case Channel::L:
        case Channel::SL:
        case Channel::SBL:
            addRoutes(pInput, subLs, gain);
            break;
            //Route to right sub channels
        case Channel::R:
        case Channel::SR:
        case Channel::SBR:
            addRoutes(pInput, subRs, gain);
            break;
            //Route to both sub channels
        case Channel::C:
        case Channel::SW:
            addRoutes(pInput, subLs, gain + BASS_TO_STEREO_GAIN);
            addRoutes(pInput, subRs, gain + BASS_TO_STEREO_GAIN);
        }
    }
    //Route to mono sub channels
    else {
        addRoutes(pInput, subs, gain);
    }
}

const std::unordered_map<Channel, SpeakerType> Config::parseChannels(const JsonNode *pBasicNode, const double stereoBass, std::vector<Channel> &subs, std::vector<Channel> &subLs, std::vector<Channel> &subRs, std::vector<Channel> &smalls, const std::string &path) {
    std::unordered_map<Channel, SpeakerType> result;
    //Front speakers are always enabled and normal sepakers
    parseChannel(result, pBasicNode, "front", { Channel::L , Channel::R }, { SpeakerType::LARGE, SpeakerType::SMALL }, path);
    //Subwoofer is always sub or off.
    parseChannel(result, pBasicNode, "subwoofer", { Channel::SW }, { SpeakerType::SUB, SpeakerType::OFF }, path);
    //Rest of channels can be either speaker, sub or off
    const std::vector<SpeakerType> allowed = { SpeakerType::LARGE, SpeakerType::SMALL, SpeakerType::SUB, SpeakerType::OFF };
    parseChannel(result, pBasicNode, "center", { Channel::C }, allowed, path);
    parseChannel(result, pBasicNode, "surround", { Channel::SL, Channel::SR }, allowed, path);
    parseChannel(result, pBasicNode, "surroundBack", { Channel::SBL, Channel::SBR }, allowed, path);

    //Fill type lists
    for (auto &e : result) {
        switch (e.second) {
        case SpeakerType::SMALL:
            smalls.push_back(e.first);
            break;
        case SpeakerType::SUB:
            if (stereoBass) {
                switch (e.first) {
                case Channel::C:
                case Channel::SL:
                case Channel::SBL:
                    subLs.push_back(e.first);
                    break;
                case Channel::SW:
                case Channel::SR:
                case Channel::SBR:
                    subRs.push_back(e.first);
                    break;
                }
            }
            else {
                subs.push_back(e.first);
            }
            break;
        }
    }

    if (getUseSubwoofers(subs, subLs, subRs)) {
        if (subLs.size() != subRs.size()) {
            throw Error("Config(%s) - Can't use stereo bass if not both center(C) and subwoofer(SW) channels are used as subwoofers", path.c_str());
        }
    }
    else {
        if (result.at(Channel::L) == SpeakerType::SMALL) {
            throw Error("Config(%s) - Can't use small front speakers with no subwoofer", path.c_str());
        }
        if (stereoBass) {
            LOG_WARN("WARNING: Config(%s) -  Can't use stereo bass if no subwoofer channels are used", path.c_str());
        }
    }

    return result;
}

void Config::parseChannel(std::unordered_map<Channel, SpeakerType> &result, const JsonNode *pNode, const std::string &field, const std::vector<Channel> &channels, const std::vector<SpeakerType> &allowed, const std::string &path) const {
    //Use give value
    if (pNode->has(field)) {
        std::string typeStr;
        const SpeakerType type = getSpeakerType(pNode, field, typeStr, path);
        const std::string myPath = path + "/" + field;
        if (std::find(allowed.begin(), allowed.end(), type) == allowed.end()) {
            throw Error("Config(%s) - Speaker type '%s' is not allowed", myPath.c_str(), typeStr.c_str());
        }
        for (const Channel channel : channels) {
            //Set default off in case contiue below is triggered.
            result[channel] = SpeakerType::OFF; 
            //Speaker is set to playing but doesnt exist in render device
            if ((size_t)channel >= _numChannelsOut && type != SpeakerType::OFF) {
                LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", myPath.c_str(), Channels::toString(channel).c_str());
                continue;
            }
            //Speaker is set as a in->out type ie large and small but deosnt exist in capture device.
            if ((size_t)channel >= _numChannelsIn && (type == SpeakerType::LARGE || type == SpeakerType::SMALL)) {
                LOG_WARN("WARNING: Config(%s) - Capture device doesn't have channel '%s'", myPath.c_str(), Channels::toString(channel).c_str());
                continue;
            }
            result[channel] = type;
        }
    }
    //Default value
    else {
        for (const Channel channel : channels) {
            //Render device has this channel. Use default value.
            if ((size_t)channel < _numChannelsOut) {
                //First value is the default one.
                result[channel] = allowed[0];
            }
            //Render device doesnt have this channel. Set it to off
            else {
                result[channel] = SpeakerType::OFF;
            }
        }
    }
}

void Config::addRoutes(Input *pInput, const std::vector<Channel> channels, const double gain) const {
    for (const Channel channel : channels) {
        addRoute(pInput, channel, gain);
    }
}

void Config::addRoute(Input *pInput, const Channel channel, const double gain, const bool addLP) const {
    if ((size_t)channel >= _numChannelsOut) {
        LOG_WARN("WARNING: Config(Basic/channels) - Render device doesn't have channel '%s'", Channels::toString(channel).c_str());
        return;
    }
    Route *pRoute = new Route(channel);
    if (gain) {
        pRoute->addFilter(new FilterGain(gain));
    }
    if (addLP) {
        FilterBiquad *pFilterBiquad = new FilterBiquad(_sampleRate);
        std::string lpPath = "basic";
        parseCrossover(true, pFilterBiquad, _pLpFilter, lpPath);
        pRoute->addFilter(pFilterBiquad);
    }
    pInput->addRoute(pRoute);
}

const SpeakerType Config::addRoute(const std::unordered_map<Channel, SpeakerType> &channelsMap, Input *pInput, const std::vector<Channel> &channels) const {
    for (const Channel channel : channels) {
        if (channelsMap.find(channel) != channelsMap.end()) {
            const SpeakerType type = channelsMap.at(channel);
            if (type == SpeakerType::LARGE || type == SpeakerType::SMALL) {
                addRoute(pInput, channel);
                return type;
            }
        }
    }
    throw Error("No channel found for addRoute");
}

void Config::addIfRoute(Input *pInput, const Channel channel) const {
    Route *pRoute = new Route(channel);
    pRoute->addCondition(Condition(ConditionType::SILENT, (int)channel));
    pInput->addRoute(pRoute);
}

const std::vector<Channel> Config::getChannelsByType(const std::unordered_map<Channel, SpeakerType> &channelsMap, const SpeakerType targetType) const {
    std::vector<Channel> result;
    for (const auto &e : channelsMap) {
        if (e.second == targetType) {
            result.push_back(e.first);
        }
    }
    return result;
}

const double Config::getLfeGain(const JsonNode *pBasicNode, const bool useSubwoofers, const bool stereoBass, const bool hasSmalls, const std::string &path) const {
    const double lfeGain = tryGetDoubleValue(pBasicNode, "lfeGain", path);
    if (useSubwoofers) {
        //Playing subwoofer and no small speakers. IE LFE is not going to get mixed with other channels.
        if (!hasSmalls) {
            return 0;
        }
    }
    return lfeGain + LFE_GAIN;
}

const bool Config::getUseSubwoofers(const std::vector<Channel> &subs, const std::vector<Channel> &subLs, const std::vector<Channel> &subRs) const {
    return subs.size() || subLs.size() || subRs.size();
}
