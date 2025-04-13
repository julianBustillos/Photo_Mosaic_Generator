#pragma once

#include <exception>
#include <string>


class CustomException : public std::exception
{
public:
    enum Level
    {
        HELP,
        NORMAL,
        ERROR
    };

public:
    CustomException(std::string message, Level level) : exception(message.c_str()), _level(level) {};
    ~CustomException() {};
    Level getLevel() { return _level; };

private:
    Level _level;
};
