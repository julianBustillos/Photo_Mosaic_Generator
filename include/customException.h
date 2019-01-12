#pragma once

#include <exception>
#include <string>


class CustomException : public std::exception {
public:
	enum Level {
		NORMAL,
		ERROR,
		HELP
	};

public:
	CustomException(std::string message, Level level) : _message(message), _level(level) {};
	~CustomException() {};
	const char * what() const throw () { return _message.c_str(); };
	Level getLevel() { return _level; };

private:
	std::string _message;
	Level _level;
};
