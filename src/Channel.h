#pragma once
#include <string>

using std::string;

enum class Channel {
    L, R, C, SW, SBL, SBR, SL, SR, CHANNEL_NULL
};

namespace Channels {

    const Channel fromString(const string &str);
    const string toString(const Channel channel);
    const string toString(const size_t channelIndex);

};
