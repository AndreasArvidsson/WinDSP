#pragma once
#include <string>

enum class Channel {
    L, R, C, SW, SBL, SBR, SL, SR
};

namespace Channels {

    const Channel fromString(const std::string &str);
    const std::string toString(const Channel channel);
    const std::string toString(const size_t channelIndex);

};
