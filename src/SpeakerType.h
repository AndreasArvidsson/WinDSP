#pragma once
#include <string>

using std::string;

enum class SpeakerType {
    LARGE, SMALL, SUB, OFF
};

namespace SpeakerTypes {

    const SpeakerType fromString(const string &str);
    const string toString(const SpeakerType speakerType);

};
