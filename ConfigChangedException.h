/*
    This exception is thrown to restart the application and read the config file again

    Author: Andreas Arvidsson
    Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <exception>

class ConfigChangedException : public std::exception {
public:

    ConfigChangedException();
    ConfigChangedException(const char input);

    const bool hasInput() const;
    const char getInput() const;

private:
    char _input;

};
