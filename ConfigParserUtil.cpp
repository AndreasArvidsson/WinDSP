#include "Config.h"
#include "JsonNode.h"
#include "FilterType.h"
#include "SpeakerType.h"
#include "SubType.h"

#define REF_FIELD "#ref"

/* ***** JSON NODES ***** */

const JsonNode* Config::tryGetNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    const JsonNode *pResult = pNode->path(field);
    return _tryGetNodeInner(pResult, path, field);
}

const JsonNode* Config::tryGetNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    const JsonNode *pResult = pNode->path(index);
    return _tryGetNodeInner(pResult, path, std::to_string(index));
}

const JsonNode* Config::getNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    return _getNodeInner(tryGetNode(pNode, field, path), path);
}

const JsonNode* Config::getNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    return _getNodeInner(tryGetNode(pNode, index, path), path);
}

const JsonNode* Config::_tryGetNodeInner(const JsonNode *pNode, std::string &path, const std::string &appendPath) const {
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

const JsonNode* Config::_getNodeInner(const JsonNode *pNode, std::string &path) const {
    if (pNode->isMissingNode()) {
        throw Error("Config(%s) - Field is required", path.c_str());
    }
    return pNode;
}

const JsonNode* Config::_getReference(const JsonNode *pParent, std::string &path) const {
    const std::string refPath = getTextValue(pParent, REF_FIELD, path);
    path = path + "/" + REF_FIELD;

    if (pParent->size() > 1) {
        throw Error("Config(%s) - Can't combine reference with other fields", path.c_str());
    }

    JsonNode *pRefNode = _pJsonNode;
    char buf[500];
    strcpy(buf, refPath.c_str());
    char *part = strtok(buf, "/");
    while (part != NULL) {
        pRefNode = pRefNode->path(part);
        part = strtok(NULL, "/");
    }
    if (pRefNode->isMissingNode()) {
        throw Error("Config(%s) - Can't find reference '%s'", path.c_str(), refPath.c_str());
    }
    path = path + " -> " + refPath;
    return pRefNode;
}

/* ***** OBJECT NODE ***** */

const JsonNode* Config::tryGetObjectNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    return validateObjectNode(tryGetNode(pNode, field, path), path, true);
}

const JsonNode* Config::tryGetObjectNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    return validateObjectNode(tryGetNode(pNode, index, path), path, true);
}

const JsonNode* Config::getObjectNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    return validateObjectNode(getNode(pNode, field, path), path);
}

const JsonNode* Config::getObjectNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    return validateObjectNode(getNode(pNode, index, path), path);
}

const JsonNode* Config::validateObjectNode(const JsonNode *pNode, std::string &path, const bool optional) const {
    if (pNode->isObject() || (pNode->isMissingNode() && optional)) {
        return pNode;
    }
    throw Error("Config(%s) - Field is not an object. Expected: { }", path.c_str());
}

/* ***** ARRAY NODE ***** */

const JsonNode* Config::tryGetArrayNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    return validateArrayNode(tryGetNode(pNode, field, path), path, true);
}

const JsonNode* Config::tryGetArrayNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    return validateArrayNode(tryGetNode(pNode, index, path), path, true);
}

const JsonNode* Config::getArrayNode(const JsonNode *pNode, const std::string &field, std::string &path) const {
    return validateArrayNode(getNode(pNode, field, path), path);
}

const JsonNode* Config::getArrayNode(const JsonNode *pNode, const size_t index, std::string &path) const {
    return validateArrayNode(getNode(pNode, index, path), path);
}

const JsonNode* Config::validateArrayNode(const JsonNode *pNode, std::string &path, const bool optional) const {
    if (pNode->isArray() || (pNode->isMissingNode() && optional)) {
        return pNode;
    }
    throw Error("Config(%s) - Field is not an array/list. Expected: [ ]", path.c_str());
}

/* ***** TEXT VALUE ***** */

const std::string Config::tryGetTextValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateTextValue(tryGetNode(pNode, field, myPath), myPath, true);
}

const std::string Config::tryGetTextValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateTextValue(tryGetNode(pNode, index, myPath), myPath, true);
}

const std::string Config::getTextValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateTextValue(getNode(pNode, field, myPath), myPath);
}

const std::string Config::getTextValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateTextValue(getNode(pNode, index, myPath), myPath);
}

const std::string Config::validateTextValue(const JsonNode *pNode, const std::string &path, const bool optional) const {
    if (pNode->isText() || (pNode->isMissingNode() && optional)) {
        return pNode->textValue();
    }
    throw Error("Config(%s) - Field is not a text string. Expected: \" \"", path.c_str());
}

/* ***** BOOL VALUE ***** */

const bool Config::tryGetBoolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateBoolValue(tryGetNode(pNode, field, myPath), myPath, true);
}

const bool Config::tryGetBoolValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateBoolValue(tryGetNode(pNode, index, myPath), myPath, true);
}

const bool Config::getBoolValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateBoolValue(getNode(pNode, field, myPath), myPath);
}

const bool Config::getBoolValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateBoolValue(getNode(pNode, index, myPath), myPath);
}

const bool Config::validateBoolValue(const JsonNode *pNode, const std::string &path, const bool optional) const {
    if (pNode->isBoolean() || (pNode->isMissingNode() && optional)) {
        return pNode->boolValue();
    }
    throw Error("Config(%s) - Field is not a boolean. Expected: true/false", path.c_str());
}

/* ***** DOUBLE VALUE ***** */

const double Config::tryGetDoubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateDoubleValue(tryGetNode(pNode, field, myPath), myPath, true);
}

const double Config::tryGetDoubleValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateDoubleValue(tryGetNode(pNode, index, myPath), myPath, true);
}

const double Config::getDoubleValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateDoubleValue(getNode(pNode, field, myPath), myPath);
}

const double Config::getDoubleValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateDoubleValue(getNode(pNode, index, myPath), myPath);
}

const double Config::validateDoubleValue(const JsonNode *pNode, const std::string &path, const bool optional) const {
    if (pNode->isNumber() || (pNode->isMissingNode() && optional)) {
        return pNode->doubleValue();
    }
    throw Error("Config(%s) - Field is not a number", path.c_str());
}

/* ***** INT VALUE ***** */

const int Config::tryGetIntValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateIntValue(tryGetNode(pNode, field, myPath), myPath, true);
}

const int Config::tryGetIntValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateIntValue(tryGetNode(pNode, index, myPath), myPath, true);
}

const int Config::getIntValue(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    std::string myPath = path;
    return validateIntValue(getNode(pNode, field, myPath), myPath);
}

const int Config::getIntValue(const JsonNode *pNode, const size_t index, const std::string &path) const {
    std::string myPath = path;
    return validateIntValue(getNode(pNode, index, myPath), myPath);
}

const int Config::validateIntValue(const JsonNode *pNode, const std::string &path, const bool optional) const {
    if (pNode->isInteger() || (pNode->isMissingNode() && optional)) {
        return pNode->intValue();
    }
    throw Error("Config(%s) - Field is not an integer number", path.c_str());
}


/* ***** ENUM ***** */

const SpeakerType Config::getSpeakerType(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    const std::string str = getTextValue(pNode, field, path);
    try {
        return SpeakerTypes::fromString(str);
    }
    catch (const std::exception &e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), field.c_str(), e.what());
    }
}

const FilterType Config::getFilterType(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    const std::string str = getTextValue(pNode, field, path);
    try {
        return FilterTypes::fromString(str);
    }
    catch (const std::exception &e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), field.c_str(), e.what());
    }
}

const SubType Config::getSubType(const JsonNode *pNode, const std::string &field, const std::string &path) const {
    const std::string str = getTextValue(pNode, field, path);
    try {
        return SubTypes::fromString(str);
    }
    catch (const std::exception &e) {
        throw Error("Config(%s/%s) - %s", path.c_str(), field.c_str(), e.what());
    }
}