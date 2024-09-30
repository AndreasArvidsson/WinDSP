/*
    This class represents a single routing condition
    Used to evaluate conditions set on input routes

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <memory>

using std::unique_ptr;

enum class ConditionType {
    SILENT
};

class Condition {
public:

    static void init(const size_t numChannels);
    static void destroy();
    static const bool isChannelUsed(const size_t index);
    static void setIsChannelUsed(const size_t index, const bool isUsed);

    Condition(const ConditionType type, const int value);

    const bool eval() const;

private:
    static unique_ptr<bool[]> _pUsedChannels;

    ConditionType _type;
    int _value;

};

