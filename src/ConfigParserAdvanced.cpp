#include "Config.h"
#include "JsonNode.h"
#include "Input.h"
#include "WinDSPLog.h"
#include "Channel.h"

using std::move;

void Config::parseAdvanced() {
    string path = "";
    //Iterate inputs and set routes
    const shared_ptr<JsonNode> pInputs = tryGetObjectNode(_pJsonNode, "advanced", path);
    for (const string &channelName : pInputs->getOrder()) {
        parseInput(pInputs, channelName, path);
    }
    //Add default in/out route to missing
    for (size_t i = 0; i < _numChannelsIn; ++i) {
        if (!_inputs[i].isDefined()) {
            //Output exists. Route to output
            if (i < _numChannelsOut) {
                _inputs[i] = Input((Channel)i, (Channel)i);
            }
            //Output doesn't exists. Add default non-route input.
            else {
                _inputs[i] = Input((Channel)i);
            }
        }
    }
}

void Config::parseInput(const shared_ptr<JsonNode>& pInputs, const string &channelName, string path) {
    const Channel channelIn = Channels::fromString(channelName);
    const size_t channelIndex = (size_t)channelIn;
    const shared_ptr<JsonNode> pRoutes = tryGetArrayNode(pInputs, channelName, path);
    if (channelIndex >= _numChannelsIn) {
        if (pRoutes->size() > 0) {
            LOG_WARN("WARNING: Config(%s) - Capture device doesn't have channel '%s'", path.c_str(), channelName.c_str());
        }
        return;
    }
    Input input(channelIn);
    for (size_t i = 0; i < pRoutes->size(); ++i) {
        parseRoute(input, pRoutes, i, path);
    }
    _inputs[channelIndex] = move(input);
}

void Config::parseRoute(Input& input, const shared_ptr<JsonNode>& pRoutes, const size_t index, string path) {
    const shared_ptr<JsonNode> pRouteNode = getObjectNode(pRoutes, index, path);
    //If route have no out channel it's the same thing as no route at all.
    if (pRouteNode->has("out")) {
        const string outPath = path + "/out";
        const string channelName = getTextValue(pRouteNode, "out", path);
        const Channel channelOut = Channels::fromString(channelName);
        if ((size_t)channelOut >= (int)_numChannelsOut) {
            LOG_WARN("WARNING: Config(%s) - Render device doesn't have channel '%s'", outPath.c_str(), channelName.c_str());
            return;
        }
        Route route(channelOut);
        vector<unique_ptr<Filter>> filters = parseFilters(pRouteNode, path);
        route.addFilters(filters);
        parseConditions(route, pRouteNode, path);
        input.addRoute(route);
    }
}

void Config::parseConditions(Route& route, const shared_ptr<JsonNode>& pRouteNode, string path) {
    if (pRouteNode->has("if")) {
        const shared_ptr<JsonNode> pIfNode = getObjectNode(pRouteNode, "if", path);
        if (!pIfNode->has("silent")) {
            throw Error("Config(%s) - Unknown if condition", path.c_str());
        }
        const string channelName = getTextValue(pIfNode, "silent", path);
        const size_t channel = (size_t)Channels::fromString(channelName);
        route.addCondition(Condition(ConditionType::SILENT, (int)channel));
        _useConditionalRouting = true;
    }
}