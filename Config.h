/*
	This class represents the config file
	Used to load, parse and evaluate the JSON config

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <vector>
#include "DSP.h"
#include "Input.h"
#include "Output.h"
#include "AudioDevice.h"
#include "File.h"
#include "JsonNode.h"

enum FilterType;

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
	const bool useAsioRenderer() const;
	const bool startWithOS() const;
	const std::string getChannelName(const size_t channelIndex) const;
	const uint32_t getNumChannelsRender(const uint32_t capacity) const;
	const std::string getDescription() const;
	const bool hasDescription() const;

	inline const bool Config::hasChanged() const {
		return _lastModified != _configFile.getLastModifiedTime();
	}

	inline const bool useConditionalRouting() const {
		return _useConditionalRouting;
	}

private:
	File _configFile;
	JsonNode *_pJsonNode;
	std::vector<JsonNode*> _tmpJsonNodes;
	std::vector<Input*> _inputs;
	std::vector<Output*> _outputs;
	std::vector<std::string> _channelNames;
	std::string _captureDeviceName, _renderDeviceName;
	uint32_t _sampleRate, _numChannelsIn, _numChannelsOut, _numChannelsRender;
	time_t _lastModified;
	bool _hide, _minimize, _useConditionalRouting, _useAsioRenderer, _startWithOS;

	void load();
	void save();

	void parseDevices();
	void parseMisc();
	void parseInputs();
	void parseInput(const JsonNode *pInputs, const std::string &channelName, std::string path);
	void parseRoute(std::vector<Route*> &routes, const JsonNode *pRoutes, const size_t index, std::string path);
	void parseConditions(Route *pRoute, const JsonNode *pRouteNode, std::string path);
	void parseOutputs();
	void parseOutput(const JsonNode *pOutputs, const std::string &channelName, std::string path);
	void parseOutputFork(Output *pOutput, const JsonNode *pForkNode, std::string path);

	void validateLevels(const std::string &path) const;
	const double getFilterGainSum(const std::vector<Filter*> &filters, double startLevel = 1.0) const;

	//void select();
	void setDevices();
	const size_t getSelection(const size_t start, const size_t end, const size_t blacklist = -1) const;

	const size_t getChannelIndex(const std::string &channelName, const std::string &path) const;
	const std::string getChannelName(const size_t channelIndex, const std::string &path) const;

	void parseFilters(std::vector<Filter*> &filters, const JsonNode *pNode, const std::string path);
	void parseFilter(std::vector<Filter*> &filters, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseCrossover(const bool isLowPass, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseShelf(const bool isLowShelf, BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parsePEQ(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseBandPass(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseNotch(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseLinkwitzTransform(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseBiquad(BiquadFilter *pBiquadFilter, const JsonNode *pFilterNode, const std::string path) const;
	void parseGain(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path);
	void parseDelay(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path);
	void parseInvertPolarity(std::vector<Filter*> &filters, const JsonNode *pNode, std::string path) const;
	void parseFir(std::vector<Filter*> &filters, const JsonNode *pFilterNode, std::string path) const;
	void parseFirTxt(std::vector<Filter*> &filters, const File &file, std::string path) const;
	void parseFirWav(std::vector<Filter*> &filters, const File &file, std::string path) const;

	const JsonNode* getNode(const JsonNode *pNode, const std::string &field, std::string &path);
	const JsonNode* getNode(const JsonNode *pNode, const size_t index, std::string &path);
	const JsonNode* getReference(const JsonNode *pRefNode, std::string &path) const;
	const JsonNode* getField(const JsonNode *pNode, const std::string &field, const std::string &path) const;
	const JsonNode* getEnrichedReference(const JsonNode *pRefNode, const JsonNode *pOrgNode);

	const double doubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
	const double doubleValue(const JsonNode *pNode, const std::string &path) const;
	const int intValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
	const int intValue(const JsonNode *pNode, const std::string &path) const;
	const std::string textValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
	const std::string textValue(const JsonNode *pNode, const std::string &path) const;
	const bool boolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const;
	const bool boolValue(const JsonNode *pNode, const std::string &path) const;

};