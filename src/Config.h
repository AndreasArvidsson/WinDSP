/*
    This class represents the config file
    Used to load, parse and evaluate the JSON config

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include "File.h"
#include "Input.h"
#include "Output.h"

using std::string;
using std::vector;
using std::unordered_map;
using std::shared_ptr;
using std::unique_ptr;

class Route;
class Filter;
class FilterBiquad;
class FilterGain;
class JsonNode;
enum class SpeakerType;
enum class CrossoverType;
enum class FilterType;
enum class Channel;

class Config {
public:

    Config(const string& path);

    void init(const uint32_t sampleRate, const uint32_t numChannelsIn, const uint32_t numChannelsOut);
    const string getCaptureDeviceName() const;
    const string getRenderDeviceName() const;
    vector<Input>& getInputs();
    vector<Output>& getOutputs();
    const bool hide() const;
    const bool minimize() const;
    const bool startWithOS() const;
    const string getDescription() const;
    const bool hasDescription() const;
    const bool inDebug() const;
    const bool useAsioRenderDevice() const;
    const uint32_t getAsioBufferSize() const;
    const uint32_t getAsioNumChannels() const;
    const bool useConditionalRouting() const;
    const bool hasChanged() const;
    void printConfig() const;

private:
    vector<Input> _inputs;
    vector<Output> _outputs;
    unordered_map<Channel, bool> _addLpTo, _addHpTo;
    File _configFile;
    shared_ptr<JsonNode> _pJsonNode, _pLpFilter, _pHpFilter;
    string _captureDeviceName, _renderDeviceName;
    uint32_t _sampleRate, _numChannelsIn, _numChannelsOut, _asioBufferSize, _asioNumChannels;
    time_t _lastModified;
    bool _hide, _minimize, _useConditionalRouting, _startWithOS, _addAutoGain, _debug, _useAsioRenderDevice;

    /* ********* Config.cpp ********* */

    void load();
    void save();
    const double getFiltersLevelSum(const vector<unique_ptr<Filter>>& filters, double startLevel = 1.0) const;
    const size_t getSelection(const size_t start, const size_t end, const size_t blacklist = -1) const;
    void printRouteConfig(const string& prefix, Channel channel, const vector<unique_ptr<Filter>>& filters, const vector<unique_ptr<Filter>>& postFilters, const bool hasConditions = false) const;
    FilterGain* getGainFilter(const vector<unique_ptr<Filter>>& filters);

    /* ********* ConfigParser.cpp ********* */

    void parseDevices();
    void setDevices();
    void parseMisc();
    void parseRouting();
    void parseOutputs();
    void parseOutput(const shared_ptr<JsonNode>& pOutputs, const size_t index, string path);
    const Channel getOutputChannel(const string& channelName, const string& path) const;
    const vector<Channel> getOutputChannels(const shared_ptr<JsonNode>& pOutputNode, const string& path) const;
    void validateLevels(const string& path);

    /* ********* ConfigParserBasic.cpp ********* */

    void parseBasic();
    const unordered_map<Channel, SpeakerType> parseChannels(const shared_ptr<JsonNode>& pBasicNode, const double stereoBass, vector<Channel>& subs, vector<Channel>& subLs, vector<Channel>& subRs, vector<Channel>& smalls, const string& path) const;
    void parseChannel(unordered_map<Channel, SpeakerType>& result, const shared_ptr<JsonNode>& pNode, const string& field, const vector<Channel>& channels, const vector<SpeakerType>& allowed, const string& path) const;
    const vector<Channel> getChannelsByType(const unordered_map<Channel, SpeakerType>& channelsMap, const SpeakerType targetType) const;
    const double getLfeGain(const shared_ptr<JsonNode>& pBasicNode, const bool useSubwoofers, const bool hasSmalls, const string& path) const;
    void routeChannels(const unordered_map<Channel, SpeakerType>& channelsMap, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain);
    void addBassRoute(Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain) const;
    void addSwRoute(Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double gain) const;
    void addFrontBassRoute(Input& input, const bool stereoBass, const double gain) const;
    void addRoutes(Input& input, const vector<Channel> channels, const double gain) const;
    void addRoute(Input& input, const Channel channel, const double gain = 0, const bool addLP = false) const;
    const SpeakerType addRoute(const unordered_map<Channel, SpeakerType>& channelsMap, Input& input, const vector<Channel>& channels) const;
    void downmix(const unordered_map<Channel, SpeakerType>& channelsMap, Input& input, const bool stereoBass, const vector<Channel> subs, const vector<Channel> subLs, const vector<Channel> subRs, const double lfeGain) const;
    const bool getUseSubwoofers(const vector<Channel>& subs, const vector<Channel>& subLs, const vector<Channel>& subRs) const;
    void parseExpandSurround(const shared_ptr<JsonNode>& pBasicNode, const unordered_map<Channel, SpeakerType>& channelsMap, const string& path);
    void addIfRoute(Input& input, const Channel channel) const;
    void parseBasicCrossovers(const shared_ptr<JsonNode>& pBasicNode, const unordered_map<Channel, SpeakerType>& channelsMap, const string& path);
    shared_ptr<JsonNode> parseBasicCrossover(const shared_ptr<JsonNode>& pBasicNode, const string& field, const CrossoverType crossoverType, const double freq, const int order, const string& path) const;

    /* ********* ConfigParserAdvanced.cpp ********* */

    void parseAdvanced();
    void parseInput(const shared_ptr<JsonNode>& pInputs, const string& channelName, string path);
    void parseRoute(Input& input, const shared_ptr<JsonNode>& pRoutes, const size_t index, string path);
    void parseConditions(Route& route, const shared_ptr<JsonNode>& pRouteNode, string path);

    /* ********* ConfigParserFilter.cpp ********* */

    vector<unique_ptr<Filter>> parseFilters(const shared_ptr<JsonNode>& pNode, const string& path, const int outputChannel = -1) const;
    vector<unique_ptr<Filter>> parsePostFilters(const shared_ptr<JsonNode>& pNode, const string& path) const;
    void parseFilter(vector<unique_ptr<Filter>>& filters, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseCrossover(const bool isLowPass, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseShelf(const bool isLowShelf, FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parsePEQ(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseBandPass(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseNotch(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseLinkwitzTransform(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseBiquad(FilterBiquad* pFilterBiquad, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseGain(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, const string& path) const;
    void parseDelay(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, string path) const;
    void parseCompression(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, string path) const;
    void parseCancellation(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pNode, const string& path) const;
    void parseFir(vector<unique_ptr<Filter>>& filters, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void parseFirTxt(vector<unique_ptr<Filter>>& filters, const File& file, const string& path) const;
    void parseFirWav(vector<unique_ptr<Filter>>& filters, const File& file, const string& path) const;
    void applyCrossoversMap(FilterBiquad* pFilterBiquad, const Channel channel, const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    void applyCrossoversMap(vector<unique_ptr<Filter>>& filters, const Channel channel) const;
    const double getQOffset(const shared_ptr<JsonNode>& pFilterNode, const string& path) const;
    const vector<double> getQValues(const shared_ptr<JsonNode>& pFilterNode, const int order, const string& path) const;
    const bool hasCrossoverFilter(const shared_ptr<JsonNode>& pFiltersNode, const bool isLowPass, const string& path) const;

    /* ********* ConfigParserUtil.cpp ********* */

    const shared_ptr<JsonNode> tryGetNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> tryGetNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> getNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> getNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> tryGetObjectNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> tryGetObjectNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> getObjectNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> getObjectNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> _validateObjectNode(const shared_ptr<JsonNode>& node, string& path, const bool optional = false) const;
    const shared_ptr<JsonNode> tryGetArrayNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> tryGetArrayNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> getArrayNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const;
    const shared_ptr<JsonNode> getArrayNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const;
    const shared_ptr<JsonNode> _validateArrayNode(const shared_ptr<JsonNode>& node, string& path, const bool optional = false) const;
    const shared_ptr<JsonNode> _tryGetNodeInner(const shared_ptr<JsonNode>& node, string& path, const string& appendPath) const;
    const shared_ptr<JsonNode> _getNodeInner(const shared_ptr<JsonNode>& node, string& path) const;
    const shared_ptr<JsonNode> _getReference(const shared_ptr<JsonNode>& node, string& path) const;
    const string tryGetTextValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const string tryGetTextValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const string getTextValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const string getTextValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const string _validateTextValue(const shared_ptr<JsonNode>& node, const string& path, const bool optional = false) const;
    const bool tryGetBoolValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const bool tryGetBoolValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const bool getBoolValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const bool getBoolValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const bool _validateBoolValue(const shared_ptr<JsonNode>& node, const string& path, const bool optional = false) const;
    const double tryGetDoubleValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const double tryGetDoubleValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const double getDoubleValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const double getDoubleValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const double _validateDoubleValue(const shared_ptr<JsonNode>& node, const string& path, const bool optional = false) const;
    const int tryGetIntValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const int tryGetIntValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const int getIntValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const;
    const int getIntValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const;
    const int _validateIntValue(const shared_ptr<JsonNode>& node, const string& path, const bool optional = false) const;
    const SpeakerType getSpeakerType(const shared_ptr<JsonNode>& pNode, const string& field, const string& path) const;
    const FilterType getFilterType(const shared_ptr<JsonNode>& pNode, const string& field, const string& path) const;
    const CrossoverType getCrossoverType(const shared_ptr<JsonNode>& pNode, const string& path) const;

    /* ********* MISC ********* */

    template<typename T>
    void addNonExisting(shared_ptr<JsonNode>& pNode, const string& key, const T& value) const {
        if (!pNode->has(key)) {
            pNode->put(key, value);
        }
    }

};