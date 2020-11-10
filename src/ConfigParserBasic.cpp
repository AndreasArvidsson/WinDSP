#include "Config.h"
#include "SpeakerType.h"
#include "CrossoverType.h"
#include "JsonNode.h"
#include <algorithm> //find
#include "WinDSPLog.h"
#include "Channel.h"
#include "Input.h"
#include "FilterGain.h"
#include "FilterBiquad.h"

using std::make_shared;
using std::make_unique;
using std::move;

#define PHANTOM_CENTER_GAIN             -3 //Gain for routing center to L/R
#define LFE_GAIN                        10 //Gain for LFE channel to match other channels
#define BASS_TO_STEREO_GAIN             -6 //Gain for routing single bass channel to two

void Config::parseBasic() {
    _addAutoGain = true;
    string path = "";
    const shared_ptr<JsonNode> pBasicNode = getObjectNode(_pJsonNode, "basic", path);
    //Get channels
    const double stereoBass = tryGetBoolValue(pBasicNode, "stereoBass", path);
    vector<Channel> subs, subLs, subRs, smalls;
    const unordered_map<Channel, SpeakerType> channelsMap = parseChannels(pBasicNode, stereoBass, subs, subLs, subRs, smalls, path);
    const bool useSubwoofers = getUseSubwoofers(subs, subLs, subRs);
    const double lfeGain = getLfeGain(pBasicNode, useSubwoofers, smalls.size(), path);
    const double centerGain = tryGetDoubleValue(pBasicNode, "centerGain", path);

    //Parse crossover config
    parseBasicCrossovers(pBasicNode, channelsMap, path);
    //Route input to output channels
    routeChannels(channelsMap, stereoBass, subs, subLs, subRs, lfeGain, centerGain);
    //Add conditional routing to surrounds
    parseExpandSurround(pBasicNode, channelsMap, path);
}

void Config::parseBasicCrossovers(const shared_ptr<JsonNode>& pBasicNode, const unordered_map<Channel, SpeakerType>& channelsMap, const string& path) {
    for (const auto& e : channelsMap) {
        switch (e.second) {
        case SpeakerType::SMALL:
            _addHpTo[e.first] = true;
            break;
        case SpeakerType::SUB:
            _addLpTo[e.first] = true;
            break;
        }
    }
    _pLpFilter = parseBasicCrossover(pBasicNode, "lowPass", CrossoverType::BUTTERWORTH, 80, 5, path);
    _pHpFilter = parseBasicCrossover(pBasicNode, "highPass", CrossoverType::BUTTERWORTH, 80, 3, path);
}

shared_ptr<JsonNode> Config::parseBasicCrossover(const shared_ptr<JsonNode>& pBasicNode, const string& field, const CrossoverType crossoverType, const double freq, const int order, const string& path) const {
    shared_ptr<JsonNode> pCrossoverNode = make_shared<JsonNode>(JsonNodeType::OBJECT);
    if (pBasicNode->has(field)) {
        string myPath = path;
        const shared_ptr<JsonNode> pOldNode = getObjectNode(pBasicNode, field, myPath);
        for (auto f : pOldNode->getFields()) {
            pCrossoverNode->put(f.first, f.second);
        }
    }
    addNonExisting(pCrossoverNode, "crossoverType", CrossoverTypes::toString(crossoverType));
    addNonExisting(pCrossoverNode, "freq", freq);
    addNonExisting(pCrossoverNode, "order", order);
    return pCrossoverNode;
}

void Config::parseExpandSurround(const shared_ptr<JsonNode>& pBasicNode, const unordered_map<Channel, SpeakerType>& channelsMap, const string& path) {
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

void Config::routeChannels(const unordered_map<Channel, SpeakerType>& channelsMap, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain, const double centerGain) {
    for (size_t i = 0; i < _numChannelsIn; ++i) {
        const Channel channel = (Channel)i;
        Input input(channel);
        const SpeakerType type = channelsMap.at(channel);
        switch (type) {
            //Route to matching speaker
        case SpeakerType::LARGE:
            addRoute(input, channel);
            break;
            //Route to matching speaker and sub/front
        case SpeakerType::SMALL:
            addRoute(input, channel);
            addBassRoute(input, stereoBass, subs, subLs, subRs, lfeGain);
            break;
            //Downmix
        case SpeakerType::OFF:
            downmix(channelsMap, input, stereoBass, subs, subLs, subRs, lfeGain, centerGain);
            break;
            //Sub or downmix
        case SpeakerType::SUB:
            if (channel == Channel::SW) {
                addSwRoute(input, stereoBass, subs, subLs, subRs, lfeGain);
            }
            else {
                downmix(channelsMap, input, stereoBass, subs, subLs, subRs, lfeGain, centerGain);
            }
            break;
        default:
            LOG_WARN("Unmanaged type '%s'", SpeakerTypes::toString(type).c_str());
        }
        _inputs[i] = move(input);
    }
}

void Config::downmix(const unordered_map<Channel, SpeakerType>& channelsMap, Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain, const double centerGain) const {
    SpeakerType type = SpeakerType::OFF;
    switch (input.getChannel()) {
    case Channel::SL:
        type = addRoute(channelsMap, input, { Channel::SBL , Channel::L });
        break;
    case Channel::SBL:
        type = addRoute(channelsMap, input, { Channel::SL , Channel::L });
        break;
    case Channel::SR:
        type = addRoute(channelsMap, input, { Channel::SBR , Channel::R });
        break;
    case Channel::SBR:
        type = addRoute(channelsMap, input, { Channel::SR , Channel::R });
        break;
    case Channel::C: {
        const double gain = PHANTOM_CENTER_GAIN + centerGain;
        addRoute(input, Channel::L, gain);
        addRoute(input, Channel::R, gain);
        type = channelsMap.at(Channel::L);
        break;
    }
    case Channel::SW:
        type = SpeakerType::SMALL;
        break;
    default:
        LOG_WARN("Unmanaged downmix channel '%s'", Channels::toString(input.getChannel()).c_str());
    }
    //Downmixed a small speaker. Add to subs as well.
    if (type == SpeakerType::SMALL) {
        addBassRoute(input, stereoBass, subs, subLs, subRs, lfeGain, centerGain);
    }
}

void Config::addBassRoute(Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain, const double centerGain) const {
    double gain;
    switch (input.getChannel()) {
    case Channel::SW:
        gain = lfeGain;
        break;
    case Channel::C:
        gain = centerGain;
        break;
    default:
        gain = 0;
    }
    if (getUseSubwoofers(subs, subLs, subRs)) {
        addSwRoute(input, stereoBass, subs, subLs, subRs, gain);
    }
    else {
        addFrontBassRoute(input, stereoBass, gain);
    }
}

void Config::addFrontBassRoute(Input& input, const bool stereoBass, const double gain) const {
    const Channel channel = input.getChannel();
    const bool addLP = (channel != Channel::SW);
    if (stereoBass) {
        switch (channel) {
            //Route to left front channel
        case Channel::SL:
        case Channel::SBL:
            addRoute(input, Channel::L, gain, addLP);
            break;
            //Route to right front channel
        case Channel::SR:
        case Channel::SBR:
            addRoute(input, Channel::R, gain, addLP);
            break;
            //Route to both front channels
        case Channel::C:
        case Channel::SW:
            addRoute(input, Channel::L, gain + BASS_TO_STEREO_GAIN, addLP);
            addRoute(input, Channel::R, gain + BASS_TO_STEREO_GAIN, addLP);
        }
    }
    //Route to both front channels
    else {
        addRoute(input, Channel::L, gain + BASS_TO_STEREO_GAIN, addLP);
        addRoute(input, Channel::R, gain + BASS_TO_STEREO_GAIN, addLP);
    }
}

void Config::addSwRoute(Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double gain) const {
    if (stereoBass) {
        switch (input.getChannel()) {
            //Route to left sub channels
        case Channel::L:
        case Channel::SL:
        case Channel::SBL:
            addRoutes(input, subLs, gain);
            break;
            //Route to right sub channels
        case Channel::R:
        case Channel::SR:
        case Channel::SBR:
            addRoutes(input, subRs, gain);
            break;
            //Route to both sub channels
        case Channel::C:
        case Channel::SW:
            addRoutes(input, subLs, gain + BASS_TO_STEREO_GAIN);
            addRoutes(input, subRs, gain + BASS_TO_STEREO_GAIN);
        }
    }
    //Route to mono sub channels
    else {
        addRoutes(input, subs, gain);
    }
}

const unordered_map<Channel, SpeakerType> Config::parseChannels(const shared_ptr<JsonNode>& pBasicNode, const double stereoBass, vector<Channel>& subs, vector<Channel>& subLs, vector<Channel>& subRs, vector<Channel>& smalls, const string& path) const {
    unordered_map<Channel, SpeakerType> result;
    //Front speakers are always enabled and normal sepakers
    parseChannel(result, pBasicNode, "front", { Channel::L , Channel::R }, { SpeakerType::LARGE, SpeakerType::SMALL }, path);
    //Subwoofer is always sub or off.
    parseChannel(result, pBasicNode, "subwoofer", { Channel::SW }, { SpeakerType::SUB, SpeakerType::OFF }, path);
    //Rest of channels can be either speaker, sub or off
    const vector<SpeakerType> allowed = { SpeakerType::LARGE, SpeakerType::SMALL, SpeakerType::SUB, SpeakerType::OFF };
    parseChannel(result, pBasicNode, "center", { Channel::C }, allowed, path);
    parseChannel(result, pBasicNode, "surround", { Channel::SL, Channel::SR }, allowed, path);
    parseChannel(result, pBasicNode, "surroundBack", { Channel::SBL, Channel::SBR }, allowed, path);

    //Fill type lists
    for (auto& e : result) {
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

void Config::parseChannel(unordered_map<Channel, SpeakerType>& result, const shared_ptr<JsonNode>& pNode, const string& field, const vector<Channel>& channels, const vector<SpeakerType>& allowed, const string& path) const {
    //Use give value
    if (pNode->has(field)) {
        const SpeakerType type = getSpeakerType(pNode, field, path);
        const string myPath = path + "/" + field;
        if (find(allowed.begin(), allowed.end(), type) == allowed.end()) {
            throw Error("Config(%s) - Speaker type '%s' is not allowed", myPath.c_str(), SpeakerTypes::toString(type).c_str());
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

void Config::addRoutes(Input& input, const vector<Channel> channels, const double gain) const {
    for (const Channel channel : channels) {
        addRoute(input, channel, gain);
    }
}

void Config::addRoute(Input& input, const Channel channel, const double gain, const bool addLP) const {
    if ((size_t)channel >= _numChannelsOut) {
        LOG_WARN("WARNING: Config(Basic/channels) - Render device doesn't have channel '%s'", Channels::toString(channel).c_str());
        return;
    }
    Route route(channel);
    if (gain) {
        route.addFilter(make_unique<FilterGain>(gain));
    }
    if (addLP) {
        unique_ptr<FilterBiquad> pFilterBiquad = make_unique<FilterBiquad>(_sampleRate);
        string lpPath = "basic";
        parseCrossover(true, pFilterBiquad.get(), _pLpFilter, lpPath);
        route.addFilter(move(pFilterBiquad));
    }
    input.addRoute(route);
}

const SpeakerType Config::addRoute(const unordered_map<Channel, SpeakerType>& channelsMap, Input& input, const vector<Channel>& channels) const {
    for (const Channel channel : channels) {
        if (channelsMap.find(channel) != channelsMap.end()) {
            const SpeakerType type = channelsMap.at(channel);
            if (type == SpeakerType::LARGE || type == SpeakerType::SMALL) {
                addRoute(input, channel);
                return type;
            }
        }
    }
    throw Error("No channel found for addRoute");
}

void Config::addIfRoute(Input& input, const Channel channel) const {
    Route route(channel);
    route.addCondition(Condition(ConditionType::SILENT, (int)channel));
    input.addRoute(route);
}

const vector<Channel> Config::getChannelsByType(const unordered_map<Channel, SpeakerType>& channelsMap, const SpeakerType targetType) const {
    vector<Channel> result;
    for (const auto& e : channelsMap) {
        if (e.second == targetType) {
            result.push_back(e.first);
        }
    }
    return result;
}

const double Config::getLfeGain(const shared_ptr<JsonNode>& pBasicNode, const bool useSubwoofers, const bool hasSmalls, const string& path) const {
    const double lfeGain = tryGetDoubleValue(pBasicNode, "lfeGain", path);
    //Playing subwoofer and no small speakers. IE LFE is not going to get mixed with other channels.
    if (useSubwoofers && !hasSmalls) {
        return 0;
    }
    return lfeGain + LFE_GAIN;
}

const bool Config::getUseSubwoofers(const vector<Channel>& subs, const vector<Channel>& subLs, const vector<Channel>& subRs) const {
    return subs.size() || subLs.size() || subRs.size();
}