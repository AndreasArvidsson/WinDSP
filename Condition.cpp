#include "Condition.h"

const time_t* Condition::_pUsedChannels = nullptr;

void Condition::init(const time_t *pUsedChannels) {
	_pUsedChannels = pUsedChannels;
}

Condition::Condition(const ConditionType type, const int value) {
	_type = type;
	_value = value;
}

const bool Condition::eval() const {
	switch (_type) {
	case ConditionType::SILENT:
		return _pUsedChannels[_value] == 0;
		break;
	default:
		throw Error("Route - Unknown condition: %d", _type);
	}
}