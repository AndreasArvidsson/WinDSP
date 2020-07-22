#include "Config.h"
#include "JsonNode.h"
#include "FilterType.h"
#include "SpeakerType.h"
#include "CrossoverType.h"

#define REF_FIELD "#ref"

using std::to_string;
using std::exception;

/* ***** JSON NODES ***** */

const shared_ptr<JsonNode> Config::tryGetNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    return _tryGetNodeInner(
        pNode->path(field),
        path,
        field
    );
}

const shared_ptr<JsonNode> Config::tryGetNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    return _tryGetNodeInner(
        pNode->path(index),
        path,
        to_string(index)
    );
}

const shared_ptr<JsonNode> Config::getNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    return _getNodeInner(
        tryGetNode(pNode, field, path),
        path
    );
}

const shared_ptr<JsonNode> Config::getNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    return _getNodeInner(
        tryGetNode(pNode, index, path),
        path
    );
}

const shared_ptr<JsonNode> Config::_tryGetNodeInner(const shared_ptr<JsonNode>& pNode, string& path, const string& appendPath) const {
    if (path.size() > 0) {
        path = path + "/" + appendPath;
    }
    else {
        path = appendPath;
    }
    if (pNode->has(REF_FIELD)) {
        return _getReference(pNode, path);
    }
    return pNode;
}

const shared_ptr<JsonNode> Config::_getNodeInner(const shared_ptr<JsonNode>& pNode, string& path) const {
    if (pNode->isMissingNode()) {
        throw Error("Config(%s) - Field is required", path.c_str());
    }
    return pNode;
}

const shared_ptr<JsonNode> Config::_getReference(const shared_ptr<JsonNode>& pNode, string& path) const {
    const string refPath = getTextValue(pNode, REF_FIELD, path);
    path = path + "/" + REF_FIELD;

    if (pNode->size() > 1) {
        throw Error("Config(%s) - Can't combine reference with other fields", path.c_str());
    }

    shared_ptr<JsonNode> refNode = _pJsonNode;
    char buf[500];
    strcpy(buf, refPath.c_str());
    char* part = strtok(buf, "/");
    while (part != NULL) {
        refNode = refNode->path(part);
        part = strtok(NULL, "/");
    }
    if (refNode->isMissingNode()) {
        throw Error("Config(%s) - Can't find reference '%s'", path.c_str(), refPath.c_str());
    }
    path = path + " -> " + refPath;
    return refNode;
}

/* ***** OBJECT NODE ***** */

const shared_ptr<JsonNode> Config::tryGetObjectNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, field, path);
    return _validateObjectNode(res, path, true);
}

const shared_ptr<JsonNode> Config::tryGetObjectNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, index, path);
    return _validateObjectNode(res, path, true);
}

const shared_ptr<JsonNode> Config::getObjectNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, field, path);
    return _validateObjectNode(res, path);
}

const shared_ptr<JsonNode> Config::getObjectNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, index, path);
    return _validateObjectNode(res, path);
}

const shared_ptr<JsonNode> Config::_validateObjectNode(const shared_ptr<JsonNode>& pNode, string& path, const bool optional) const {
    if (pNode->isObject() || (pNode->isMissingNode() && optional)) {
        return pNode;
    }
    throw Error("Config(%s) - Field is not an object. Expected: { }", path.c_str());
}

/* ***** ARRAY NODE ***** */

const shared_ptr<JsonNode> Config::tryGetArrayNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, field, path);
    return _validateArrayNode(res, path, true);
}

const shared_ptr<JsonNode> Config::tryGetArrayNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    const shared_ptr<JsonNode> res = tryGetNode(pNode, index, path);
    return _validateArrayNode(res, path, true);
}

const shared_ptr<JsonNode> Config::getArrayNode(const shared_ptr<JsonNode>& pNode, const string& field, string& path) const {
    return _validateArrayNode(
        tryGetNode(pNode, field, path),
        path
    );
}

const shared_ptr<JsonNode> Config::getArrayNode(const shared_ptr<JsonNode>& pNode, const size_t index, string& path) const {
    return _validateArrayNode(
        tryGetNode(pNode, index, path),
        path
    );
}

const shared_ptr<JsonNode> Config::_validateArrayNode(const shared_ptr<JsonNode>& pNode, string& path, const bool optional) const {
    if (pNode->isArray() || (pNode->isMissingNode() && optional)) {
        return pNode;
    }
    throw Error("Config(%s) - Field is not an array/list. Expected: [ ]", path.c_str());
}

/* ***** TEXT VALUE ***** */

const string Config::tryGetTextValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateTextValue(
        tryGetNode(pNode, field, path),
        path,
        true
    );
}

const string Config::tryGetTextValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateTextValue(
        tryGetNode(pNode, index, path),
        path,
        true
    );
}

const string Config::getTextValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateTextValue(
        getNode(pNode, field, path),
        path
    );
}

const string Config::getTextValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateTextValue(
        getNode(pNode, index, path),
        path
    );
}

const string Config::_validateTextValue(const shared_ptr<JsonNode>& pNode, const string& path, const bool optional) const {
    if (pNode->isText() || (pNode->isMissingNode() && optional)) {
        return pNode->textValue();
    }
    throw Error("Config(%s) - Field is not a text string. Expected: \" \"", path.c_str());
}

/* ***** BOOL VALUE ***** */

const bool Config::tryGetBoolValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateBoolValue(
        tryGetNode(pNode, field, path),
        path,
        true
    );
}

const bool Config::tryGetBoolValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateBoolValue(
        tryGetNode(pNode, index, path),
        path,
        true
    );
}

const bool Config::getBoolValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateBoolValue(
        getNode(pNode, field, path),
        path
    );
}

const bool Config::getBoolValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateBoolValue(
        getNode(pNode, index, path),
        path
    );
}

const bool Config::_validateBoolValue(const shared_ptr<JsonNode>& pNode, const string& path, const bool optional) const {
    if (pNode->isBoolean() || (pNode->isMissingNode() && optional)) {
        return pNode->boolValue();
    }
    throw Error("Config(%s) - Field is not a boolean. Expected: true/false", path.c_str());
}

/* ***** DOUBLE VALUE ***** */

const double Config::tryGetDoubleValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateDoubleValue(
        tryGetNode(pNode, field, path),
        path,
        true
    );
}

const double Config::tryGetDoubleValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateDoubleValue(
        tryGetNode(pNode, index, path),
        path,
        true
    );
}

const double Config::getDoubleValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateDoubleValue(
        getNode(pNode, field, path),
        path
    );
}

const double Config::getDoubleValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateDoubleValue(
        getNode(pNode, index, path),
        path
    );
}

const double Config::_validateDoubleValue(const shared_ptr<JsonNode>& pNode, const string& path, const bool optional) const {
    if (pNode->isNumber() || (pNode->isMissingNode() && optional)) {
        return pNode->doubleValue();
    }
    throw Error("Config(%s) - Field is not a number", path.c_str());
}

/* ***** INT VALUE ***** */

const int Config::tryGetIntValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateIntValue(
        tryGetNode(pNode, field, path),
        path,
        true
    );
}

const int Config::tryGetIntValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateIntValue(
        tryGetNode(pNode, index, path),
        path,
        true
    );
}

const int Config::getIntValue(const shared_ptr<JsonNode>& pNode, const string& field, string path) const {
    return _validateIntValue(
        getNode(pNode, field, path),
        path
    );
}

const int Config::getIntValue(const shared_ptr<JsonNode>& pNode, const size_t index, string path) const {
    return _validateIntValue(
        getNode(pNode, index, path), 
        path
    );
}

const int Config::_validateIntValue(const shared_ptr<JsonNode>& pNode, const string& path, const bool optional) const {
    if (pNode->isInteger() || (pNode->isMissingNode() && optional)) {
        return pNode->intValue();
    }
    throw Error("Config(%s) - Field is not an integer number", path.c_str());
}


/* ***** ENUM ***** */

const SpeakerType Config::getSpeakerType(const shared_ptr<JsonNode>& pNode, const string& field, const string& path) const {
    const string str = getTextValue(pNode, field, path);
    try {
        return SpeakerTypes::fromString(str);
    }
    catch (const exception& e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), field.c_str(), e.what());
    }
}

const FilterType Config::getFilterType(const shared_ptr<JsonNode>& pNode, const string& field, const string& path) const {
    const string str = getTextValue(pNode, field, path);
    try {
        return FilterTypes::fromString(str);
    }
    catch (const exception& e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), field.c_str(), e.what());
    }
}

const CrossoverType Config::getCrossoverType(const shared_ptr<JsonNode>& pNode, const string& path) const {
    const string str = getTextValue(pNode, "crossoverType", path);
    try {
        return CrossoverTypes::fromString(str);
    }
    catch (const exception& e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), "crossoverType", e.what());
    }
}