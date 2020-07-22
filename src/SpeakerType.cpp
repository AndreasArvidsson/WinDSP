#include "SpeakerType.h"
#include "Error.h"
#include "Str.h"

const SpeakerType SpeakerTypes::fromString(const string &strIn) {
    const string str = String::toUpperCase(strIn);
    if (str.compare("LARGE") == 0) {
        return SpeakerType::LARGE;
    }
    else if (str.compare("SMALL") == 0) {
        return SpeakerType::SMALL;
    }
    else if (str.compare("SUB") == 0) {
        return SpeakerType::SUB;
    }
    else if (str.compare("OFF") == 0) {
        return SpeakerType::OFF;
    }
    throw Error("Unknown speaker type '%s'", strIn.c_str());
}

const string SpeakerTypes::toString(const SpeakerType speakerType) {
    switch (speakerType) {
    case SpeakerType::LARGE:
        return "LARGE";
    case SpeakerType::SMALL:
        return "SMALL";
    case SpeakerType::SUB:
        return "SUB";
    case SpeakerType::OFF:
        return "OFF";
    default:
        throw Error("Unknown speaker type %d", speakerType);
    };
}