#include "Condition.h"
#include "Error.h"

using std::make_unique;

unique_ptr<bool[]> Condition::_pUsedChannels;

void Condition::init(const size_t numChannels) {
    _pUsedChannels = make_unique<bool[]>(numChannels);
    for (int i = 0; i < numChannels; ++i) {
        _pUsedChannels[i] = 0;
    }
}

void Condition::destroy() {
    _pUsedChannels = nullptr;
}

const bool Condition::isChannelUsed(const size_t index) {
    return _pUsedChannels[index];
}

void Condition::setIsChannelUsed(const size_t index, const bool isUsed) {
    _pUsedChannels[index] = isUsed;
}

Condition::Condition(const ConditionType type, const int value) {
    _type = type;
    _value = value;
}

const bool Condition::eval() const {
    switch (_type) {
    case ConditionType::SILENT:
        return !_pUsedChannels[_value];
    default:
        throw Error("Route - Unknown condition: %d", _type);
    }
}