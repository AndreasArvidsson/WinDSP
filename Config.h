/*
    This class represents the config file
    Used to load, parse and evaluate the JSON config

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <unordered_map>
#include "File.h"

class Input;
class Output;
class Route;
class Filter;
class FilterBiquad;
class JsonNode;
enum class SpeakerType;
enum class SubType;
enum class FilterType;
enum class Channel;

class Config {
public:

    Config(const std::string &path);
    ~Config();

    void init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut);
    const std::string getCaptureDeviceName() const;
    const std::string getRenderDeviceName() const;
    const std::vector<Input*>* getInputs() const;
    const std::vector<Output*>* getOutputs() const;
    const bool hide() const;
    const bool minimize() const;
    const bool startWithOS() const;
    const std::string getDescription() const;
    const bool hasDescription() const;
    const bool inDebug() const;

    inline const bool Config::hasChanged() const {
        return _lastModified != _configFile.getLastModifiedTime();
    }

    inline const bool useConditionalRouting() const {
        return _useConditionalRouting;
    }

private:
    std::vector<Input*> _inputs;
    std::vector<Output*> _outputs;
    std::unordered_map<Channel, bool> _addLpTo, _addHpTo;
    File _configFile;
    JsonNode *_pJsonNode, *_pLpFilter, *_pHpFilter;
    std::string _captureDeviceName, _renderDeviceName;
    uint32_t _sampleRate, _numChannelsIn, _numChannelsOut;
    time_t _lastModified;
    bool _hide, _minimize, _useConditionalRouting, _startWithOS, _addAutoGain, _debug;

    /* ********* Config.cpp ********* */

    void load();
    void save();
    const double getFiltersLevelSum(const std::vector<Filter*> &filters, double startLevel = 1.0) const;
    const size_t getSelection(const size_t start, const size_t end, const size_t blacklist = -1) const;
    void printConfig() const;
    void printRouteConfig(const Channel channel, const std::vector<Filter*> &filters, const std::vector<Filter*> &postFilters, const bool hasConditions = false) const;
    const bool hasGainFilter(const std::vector<Filter*> &filters) const;

    /* ********* ConfigParser.cpp ********* */

    void parseDevices();
    void setDevices();
    void parseMisc();
    void parseRouting();
    void parseOutputs();
    void parseOutput(const JsonNode *pOutputs, const size_t index, std::string path);
    const bool getOutputChannel(const JsonNode *pChannelNode, Channel &channelOut, const std::string &path) const;
    const std::vector<Channel> getOutputChannels(const JsonNode *pOutputNode, const std::string &path);
    void validateLevels(const std::string &path) const;

    /* ********* ConfigParserBasic.cpp ********* */

    void parseBasic();
    const std::unordered_map<Channel, SpeakerType> parseChannels(const JsonNode *pBasicNode, const double stereoBass, std::vector<Channel> &subs, std::vector<Channel> &subLs, std::vector<Channel> &subRs, std::vector<Channel> &smalls, const std::string &path);
    void parseChannel(std::unordered_map<Channel, SpeakerType> &result, const JsonNode *pNode, const std::string &field, const std::vector<Channel> &channels, const std::vector<SpeakerType> &allowed, const std::string &path) const;
    const std::vector<Channel> getChannelsByType(const std::unordered_map<Channel, SpeakerType> &channelsMap, const SpeakerType targetType) const;
    const double getLfeGain(const JsonNode *pBasicNode, const bool useSubwoofers, const bool stereoBass, const bool hasSmalls, const std::string &path) const;
    void routeChannels(const std::unordered_map<Channel, SpeakerType> &channelsMap, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain);
    void addBassRoute(Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const;
    void addSwRoute(Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double gain) const;
    void addFrontBassRoute(Input *pInput, const bool stereoBass, const double gain) const;
    void addRoutes(Input *pInput, const std::vector<Channel> channels, const double gain) const;
    void addRoute(Input *pInput, const Channel channel, const double gain = 0, const bool addLP = false) const;
    const SpeakerType addRoute(const std::unordered_map<Channel, SpeakerType> &channelsMap, Input *pInput, const std::vector<Channel> &channels) const;
    void downmix(const std::unordered_map<Channel, SpeakerType> &channelsMap, Input *pInput, const bool stereoBass, const std::vector<Channel> subs, const std::vector<Channel> subLs, const std::vector<Channel> subRs, const double lfeGain) const;
    const bool getUseSubwoofers(const std::vector<Channel> &subs, const std::vector<Channel> &subLs, const std::vector<Channel> &subRs) const;
    void parseExpandSurround(const JsonNode *pBasicNode, const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::string &path);
    void addIfRoute(Input *pInput, const Channel channel) const;
    void parseBasicCrossovers(JsonNode *pBasicNode, const std::unordered_map<Channel, SpeakerType> &channelsMap, const std::string &path);
    JsonNode* parseBasicCrossover(JsonNode *pBasicNode, const std::string &field, const std::string &type, const double freq, const int order, const std::string &path) const;

    /* ********* ConfigParserAdvanced.cpp ********* */

    void parseAdvanced();
    void parseInput(const JsonNode *pInputs, const std::string &channelName, std::string path);
    void parseRoute(Input *pInput, const JsonNode *pRoutes, const size_t index, std::string path);
    void parseConditions(Route *pRoute, const JsonNode *pRouteNode, std::string path);

    /* ********* ConfigParserFilter.cpp ********* */

    const std::vector<Filter*> parseFilters(const JsonNode *pNode, const std::string &path, const int outputChannel = -1);
    const std::vector<Filter*> parsePostFilters(const JsonNode *pNode, const std::string &path);
    void parseFilter(std::vector<Filter*> &filters, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const int outputChannel, const std::string &path);
    void parseCrossover(const bool isLowPass, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseShelf(const bool isLowShelf, FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parsePEQ(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseBandPass(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseNotch(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseLinkwitzTransform(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseBiquad(FilterBiquad *pFilterBiquad, const JsonNode *pFilterNode, const std::string &path) const;
    void parseGain(std::vector<Filter*> &filters, const JsonNode *pNode, const std::string &path);
    void parseDelay(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path);
    void parseCancellation(std::vector<Filter*> &filters, const JsonNode *pNode, const std::string &path) const;
    void parseFir(std::vector<Filter*> &filters, const JsonNode *pFilterNode, const std::string &path) const;
    void parseFirTxt(std::vector<Filter*> &filters, const File &file, const std::string &path) const;
    void parseFirWav(std::vector<Filter*> &filters, const File &file, const std::string &path) const;
    void updateCrossoverMaps(const bool isLP, const int outputChannel);
    void applyCrossoversMap(FilterBiquad *pFilterBiquad, const int outputChannel);

    /* ********* ConfigParserUtil.cpp ********* */

    const JsonNode* tryGetNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* tryGetNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* getNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* getNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* tryGetObjectNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* tryGetObjectNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* getObjectNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* getObjectNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* validateObjectNode(const JsonNode *pNode, std::string &path, const bool optional = false) const;
    const JsonNode* tryGetArrayNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* tryGetArrayNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* getArrayNode(const JsonNode *pNode, const std::string &field, std::string &path) const;
    const JsonNode* getArrayNode(const JsonNode *pNode, const size_t index, std::string &path) const;
    const JsonNode* validateArrayNode(const JsonNode *pNode, std::string &path, const bool optional = false) const;
    const JsonNode* _tryGetNodeInner(const JsonNode *pNode, std::string &path, const std::string &appendPath) const;
    const JsonNode* _getNodeInner(const JsonNode *pNode, std::string &path) const;
    const JsonNode* _getReference(const JsonNode *pParent, std::string &path) const;
    const std::string tryGetTextValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const std::string tryGetTextValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const std::string getTextValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const std::string getTextValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const std::string validateTextValue(const JsonNode *pNode, const std::string &path, const bool optional = false) const;
    const bool tryGetBoolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const bool tryGetBoolValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const bool getBoolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const bool getBoolValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const bool validateBoolValue(const JsonNode *pNode, const std::string &path, const bool optional = false) const;
    const double tryGetDoubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const double tryGetDoubleValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const double getDoubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const double getDoubleValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const double validateDoubleValue(const JsonNode *pNode, const std::string &path, const bool optional = false) const;
    const int tryGetIntValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const int tryGetIntValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const int getIntValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const int getIntValue(const JsonNode *pNode, const size_t index, const std::string &path) const;
    const int validateIntValue(const JsonNode *pNode, const std::string &path, const bool optional = false) const;
    const SpeakerType getSpeakerType(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const SpeakerType getSpeakerType(const JsonNode *pNode, const std::string &field, std::string &textOut, const std::string &path) const;
    const FilterType getFilterType(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const FilterType getFilterType(const JsonNode *pNode, const std::string &field, std::string &textOut, const std::string &path) const;
    const SubType getSubType(const JsonNode *pNode, const std::string &field, const std::string &path) const;
    const SubType getSubType(const JsonNode *pNode, const std::string &field, std::string &textOut, const std::string &path) const;

    /* ********* MISC ********* */

    template<typename T>
    void addNonExisting(JsonNode *pNode, const std::string &key, const T &value) const {
        if (!pNode->has(key)) {
            pNode->put(key, value);
        }
    }

};