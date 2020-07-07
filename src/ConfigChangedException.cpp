#include "ConfigChangedException.h"

ConfigChangedException::ConfigChangedException() {
    _input = '\0';
}

ConfigChangedException::ConfigChangedException(const char input) {
    this->_input = input;
}

const bool ConfigChangedException::hasInput() const {
    return _input != '\0';
}

const char ConfigChangedException::getInput() const {
    return _input;
}