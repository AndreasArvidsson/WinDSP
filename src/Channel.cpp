#include "Channel.h"
#include "Error.h"
#include "Str.h"

const Channel Channels::fromString(const string &strIn) {
    const string str = String::toUpperCase(strIn);
    if (str.compare("L") == 0) {
        return Channel::L;
    }
    else if (str.compare("R") == 0) {
        return Channel::R;
    }
    else if (str.compare("C") == 0) {
        return Channel::C;
    }
    else if (str.compare("SW") == 0) {
        return Channel::SW;
    }
    else if (str.compare("SL") == 0) {
        return Channel::SL;
    }
    else if (str.compare("SR") == 0) {
        return Channel::SR;
    }
    else if (str.compare("SBL") == 0) {
        return Channel::SBL;
    }
    else if (str.compare("SBR") == 0) {
        return Channel::SBR;
    }
    throw Error("Unknown channel '%s'", strIn.c_str());
}

const string Channels::toString(const Channel channel) {
    switch (channel) {
    case Channel::L:
        return "L";
    case Channel::R:
        return "R";
    case Channel::C:
        return "C";
    case Channel::SW:
        return "SW";
    case Channel::SL:
        return "SL";
    case Channel::SR:
        return "SR";
    case Channel::SBL:
        return "SBL";
    case Channel::SBR:
        return "SBR";
    default:
        throw Error("Unknown channel type %d", channel);
    };
}

const string Channels::toString(const size_t channelIndex) {
    return toString((Channel)channelIndex);
}