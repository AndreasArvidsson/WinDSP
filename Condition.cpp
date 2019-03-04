#include "Condition.h"
#include "Error.h"

const bool* Condition::_pUsedChannels = nullptr;

void Condition::init(const bool *pUsedChannels) {
	_pUsedChannels = pUsedChannels;
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