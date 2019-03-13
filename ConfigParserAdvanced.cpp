#include "Config.h"
#include "JsonNode.h"
#include "Input.h"
#include "WinDSPLog.h"
#include "Channel.h"

void Config::parseAdvanced() {
    std::string path = "";
    //Iterate inputs and set routes
    const JsonNode *pInputs = tryGetObjectNode(_pJsonNode, "advanced", path);
    for (const std::string &channelName : pInputs->getOrder()) {
        parseInput(pInputs, channelName, path);
    }
    //Add default in/out route to missing
    for (size_t i = 0; i < _numChannelsIn; ++i) {
        if (!_inputs[i]) {
            //Output exists. Route to output
            if (i < _numChannelsOut) {
                _inputs[i] = new Input((Channel)i);
            }
            //Output doesn't exists. Add default non-route input.
            else {
                _inputs[i] = new Input((Channel)i);
            }
        }
    }
}

void Config::parseInput(const JsonNode *pInputs, const std::string &channelName, std::string path) {
    const Channel channelIn = Channels::fromString(channelName);
    const size_t channelIndex = (size_t)channelIn;
    if (channelIndex >= _numChannelsIn) {
        LOG_WARN("WARNING: Config(%s) - Capture device doesn't have channel '%s'", path.c_str(), channelName.c_str());
        return;
    }
    _inputs[channelIndex] = new Input(channelIn);
    const JsonNode *pRoutes = tryGetArrayNode(pInputs, channelName, path);
    for (size_t i = 0; i < pRoutes->size(); ++i) {
        parseRoute(_inputs[channelIndex], pRoutes, i, path);
    }
}

void Config::parseRoute(Input *pInput, const JsonNode *pRoutes, const size_t index, std::string path) {
    const JsonNode *pRouteNode = getObjectNode(pRoutes, index, path);
    //If route have no out channel it's the same thing as no route at all.
    if (pRouteNode->has("out")) {
        const std::string outPath = path + "/out";
        const std::string channelName = getTextValue(pRouteNode, "out", path);
        const Channel channelOut = Channels::fromString(channelName);
        if ((size_t)channelOut >= (int)_numChannelsOut) {
            LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", outPath.c_str(), channelName.c_str());
            return;
        }
        Route *pRoute = new Route(channelOut);
        pRoute->addFilters(parseFilters(pRouteNode, path));
        parseConditions(pRoute, pRouteNode, path);
        pInput->addRoute(pRoute);
    }
}

void Config::parseConditions(Route *pRoute, const JsonNode *pRouteNode, std::string path) {
    if (pRouteNode->has("if")) {
        const JsonNode *pIfNode = getObjectNode(pRouteNode, "if", path);
        if (!pIfNode->has("silent")) {
            throw Error("Config(%s) - Unknown if condition", path.c_str());
        }
        const std::string channelName = getTextValue(pIfNode, "silent", path);
        const size_t channel = (size_t)Channels::fromString(channelName);
        pRoute->addCondition(Condition(ConditionType::SILENT, (int)channel));
        _useConditionalRouting = true;
    }
}