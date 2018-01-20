#pragma once
#include "Error.h"

enum class ConditionType {
	SILENT
};

class Condition {
public:

	static void init(const time_t *pUsedChannels);

	Condition(const ConditionType type, const int value);

	const bool eval() const;

private:
	static const time_t *_pUsedChannels;

	ConditionType _type;
	int _value;

};

