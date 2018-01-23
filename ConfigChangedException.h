/*
    This exception is thrown to restart the application and read the config file again

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <exception>

class ConfigChangedException : public std::exception {
public:
    char input;

    ConfigChangedException() {
        input = '\0';
    }

    ConfigChangedException(const char input) {
        this->input = input;
    }

    const bool hasInput() const {
        return input != '\0';
    }

};
