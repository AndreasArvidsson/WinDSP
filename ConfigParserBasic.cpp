#include "Config.h"
#include "SpeakerType.h"
#include "JsonNode.h"
#include <algorithm> //std::find
#include "WinDSPLog.h"
#include "Channel.h"
#include "Input.h"
#include "FilterGain.h"

void Config::parseBasic() {
    std::string path = "";
    const JsonNode *pBasicNode = getObjectNode(_pJsonNode, "basic", path);
    //Get channels
    const double swStereo = tryGetBoolValue(pBasicNode, "stereoSubwoofer", path);
    std::vector<Channel> subs, subLs, subRs, smalls;
    const std::unordered_map<Channel, SpeakerType> channelsMap = parseChannels(pBasicNode, swStereo, subs, subLs, subRs, smalls, path);
    const bool useSubwoofers = getUseSubwoofers(subs, subLs, subRs);
    //const bool swStereo = subLs.size();


    const double lfeGain = getLfeGain(pBasicNode, useSubwoofers, swStereo, smalls.size(), path);

    if (smalls.size() && !useSubwoofers) {
        throw Error("Config(%s) - Can't use small speakers with no subwoofers", path.c_str());
    }

    //Route input to output channels
    routeChannels(channelsMap, subs, subLs, subRs, lfeGain);
    //Add conditional routing to surrounds
    parseExpandSurround(pBasicNode, channelsMap, path);

    parseCrossover(pBasicNode, channelsMap, path);
}

void Config::parseCrossover(const JsonNode *pBasicNode, const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::string &path) {
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

    std::string lpPath = path;
    const JsonNode *pLpNode = tryGetObjectNode(pBasicNode, "lp", lpPath);
    _pLpFilter = new JsonNode(JsonNodeType::OBJECT);
    for (const auto &e : pLpNode->getFields()) {
        if (e.first.compare("type") == 0) {
            _pLpFilter->put("subType", e.second);
        }
        else {
            _pLpFilter->put(e.first, e.second);
        }
    }
    if (!_pLpFilter->has("subType")) {
        _pLpFilter->put("subType", "BUTTERWORTH");
    }
    if (!_pLpFilter->has("freq")) {
        _pLpFilter->put("freq", 80);
    }
    if (!_pLpFilter->has("order")) {
        _pLpFilter->put("order", 5);
    }

    std::string hpPath = path;
    const JsonNode *pHpNode = tryGetObjectNode(pBasicNode, "hp", hpPath);
    _pHpFilter = new JsonNode(JsonNodeType::OBJECT);
    for (const auto &e : pHpNode->getFields()) {
        if (e.first.compare("type") == 0) {
            _pHpFilter->put("subType", e.second);
        }
        else {
            _pHpFilter->put(e.first, e.second);
        }
    }
    if (!_pHpFilter->has("subType")) {
        _pHpFilter->put("subType", "BUTTERWORTH");
    }
    if (!_pHpFilter->has("freq")) {
        _pHpFilter->put("freq", 80);
    }
    if (!_pHpFilter->has("order")) {
        _pHpFilter->put("order", 3);
    }
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
        }
        addIfRoute(_inputs[(size_t)Channel::SL], Channel::SBL);
        addIfRoute(_inputs[(size_t)Channel::SR], Channel::SBR);
        _useConditionalRouting = true;
    }
}

void Config::routeChannels(const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) {
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
            //Route to matching speaker and sub
        case SpeakerType::SMALL:
            addRoute(pInput, channel);
            addSwRoute(pInput, subs, subLs, subRs, lfeGain);
            break;
            //Downmix
        case SpeakerType::OFF:
            downmix(channelsMap, pInput, subs, subLs, subRs, lfeGain);
            break;
            //Sub or downmix
        case SpeakerType::SUB:
            if (channel == Channel::SW) {
                addSwRoute(pInput, subs, subLs, subRs, lfeGain);
            }
            else {
                downmix(channelsMap, pInput, subs, subLs, subRs, lfeGain);
            }
            break;
        default:
            LOG_WARN("Unmanaged type '%s'", SpeakerTypes::toString(type).c_str());
        }
    }
}

void Config::downmix(const std::unordered_map<Channel, SpeakerType> &channelsMap, Input *pInput, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const {
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
        addRoute(pInput, Channel::L, -3);
        addRoute(pInput, Channel::R, -3);
        type = channelsMap.at(Channel::L);
        break;
    case Channel::SW:
        if (getUseSubwoofers(subs, subLs, subRs)) {
            type = SpeakerType::SMALL;
        }
        else {
            addRoute(pInput, Channel::L, lfeGain);
            addRoute(pInput, Channel::R, lfeGain);
        }
        break;
    default:
        LOG_WARN("Unmanaged downmix channel '%s'", Channels::toString(pInput->getChannel()).c_str());
    }
    //Downmixed to a small speaker. Add to subs as well.
    if (type == SpeakerType::SMALL) {
        addSwRoute(pInput, subs, subLs, subRs, lfeGain);
    }
}

void Config::addSwRoute(Input *pInput, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const {
    const bool useSubwoofers = getUseSubwoofers(subs, subLs, subRs);
    const bool swStereo = subLs.size();
    if (!useSubwoofers) {
        return;
    }
    double gain = 0;
    if (pInput->getChannel() == Channel::SW) {
        gain = lfeGain;
    }
    //-6dB when routing center to stereo subs
    else if (pInput->getChannel() == Channel::C && swStereo) {
        gain = -6;
    }
    if (swStereo) {
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
            addRoutes(pInput, subLs, gain);
            addRoutes(pInput, subRs, gain);
        }
    }
    else {
        //Route to mono sub channels
        addRoutes(pInput, subs, gain);
    }
}

const std::unordered_map<Channel, SpeakerType> Config::parseChannels(const JsonNode *pBasicNode, const double swStereo, std::vector<Channel> &subs, std::vector<Channel> &subLs, std::vector<Channel> &subRs, std::vector<Channel> &smalls, const std::string &path) {
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
            if (swStereo) {
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

    if (subLs.size() != subRs.size()) {
        throw Error("Config(%s) - Can't use stereo subwoofer if not both center(C) and subwoofer(SW) channels are used", path.c_str());
    }

    return result;
}

void Config::parseChannel(std::unordered_map<Channel, SpeakerType> &result, const JsonNode *pNode, const std::string &field, const std::vector<Channel> &channels, const std::vector<SpeakerType> &allowed, const std::string &path) const {
    //Use give value
    if (pNode->has(field)) {
        std::string typeStr;
        const SpeakerType type = getSpeakerType(pNode, field, typeStr, path);
        if (std::find(allowed.begin(), allowed.end(), type) == allowed.end()) {
            throw Error("Config(%s/%s) - Speaker type '%s' is not allowed", path.c_str(), field.c_str(), typeStr.c_str());
        }
        for (const Channel channel : channels) {
            //Speaker is set to playing but doesnt exist in render device
            if ((size_t)channel >= _numChannelsOut && type != SpeakerType::OFF) {
                LOG_WARN("WARNING: Config(%s/%s) - Render device doesn't have channel '%s'", path.c_str(), field.c_str(), Channels::toString(channel).c_str());
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

void Config::addRoute(Input *pInput, const Channel channel, const double gain) const {
    if ((size_t)channel >= _numChannelsOut) {
        LOG_WARN("WARNING: Config(Basic/channels) - Render device doesn't have channel '%s'", Channels::toString(channel).c_str());
        return;
    }
    Route *pRoute = new Route(channel);
    if (gain) {
        pRoute->addFilter(new FilterGain(gain));
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

const double Config::getLfeGain(const JsonNode *pBasicNode, const bool useSubwoofers, const bool swStereo, const bool hasSmalls, const std::string &path) const {
    const double lfeGain = tryGetDoubleValue(pBasicNode, "lfeGain", path);
    if (useSubwoofers) {
        //No small speakers. IE LFE is not going to get mixed with other channels.
        if (!hasSmalls) {
            return 0;
        }
        //+10db for mono sub and +4dB for stereo sub to get correct level when mixed with other channels
        return lfeGain + (swStereo ? 4 : 10);
    }
    //No sub. Downmix LFE to fronts
    else {
        return lfeGain + 4;
    }
    return 0;
}

const bool Config::getUseSubwoofers(const std::vector<Channel> &subs, const std::vector<Channel> &subLs, const std::vector<Channel> &subRs) const {
    return subs.size() || subLs.size() || subRs.size();
}