/*
	This class represents a single routing condition
	Used to evaluate conditions set on input routes

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include "Error.h"

enum class ConditionType {
	SILENT
};

class Condition {
public:

	static void init(const bool *pUsedChannels);

	Condition(const ConditionType type, const int value);

	const bool eval() const;

private:
	static const bool *_pUsedChannels;

	ConditionType _type;
	int _value;

};

