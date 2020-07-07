#pragma once
#include <string>

enum class SpeakerType {
    LARGE, SMALL, SUB, OFF
};

namespace SpeakerTypes {

    const SpeakerType fromString(const std::string &str);
    const std::string toString(const SpeakerType speakerType);

};
