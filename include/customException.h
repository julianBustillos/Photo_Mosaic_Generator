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
	CustomException(std::string message, Level level) : message(message), level(level) {};
	~CustomException() {};
	const char * what() const throw () { return message.c_str(); };
	Level getLevel() { return level; };

private:
	std::string message;
	Level level;
};
