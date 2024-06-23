#pragma once

#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>

#undef ERROR


namespace Log
{
    enum Level
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        SILENT
    };

    class Message;

    class Logger
    {
        friend class Message;

    public:
        static Logger& getInstance();

    public:
        Logger();
        ~Logger();

    public:
        void setStream(std::ostream* stream, bool owner);
        void setLevel(Level level);
        Message log(Level level);

    private:
        void output(Level& level, const std::string& message);

    private:
        static std::string levelToString(Level level);
        static std::string getDateTime();

    private:
        std::mutex _mutex;
        Level _level;
        std::ostream* _stream;
        bool _owner;
    };


    class Message
    {
        friend class Logger;

    public:
        ~Message();

    public:
        template <typename T>
        Message& operator<<(const T& value);

    private:
        Message(Logger& logger, Level& level);

    private:
        Logger& _logger;
        Level _level;
        std::stringstream _stream;
    };
};

inline Log::Logger& Log::Logger::getInstance()
{
    static Logger logger;
    return logger;
}

inline Log::Logger::Logger() :
    _level(SILENT), _stream(nullptr), _owner(false)
{
}

inline Log::Logger::~Logger()
{
    setStream(nullptr, false);
}

inline void Log::Logger::setStream(std::ostream* stream, bool owner)
{
    const std::lock_guard<std::mutex> lock(_mutex);

    if (_owner)
        delete _stream;

    _stream = stream;
    _owner = owner;
}

inline void Log::Logger::setLevel(Level level)
{
    _level = level;
}

inline Log::Message Log::Logger::log(Level level)
{
    return Message(*this, level);
}

inline void Log::Logger::output(Level& level, const std::string& message)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    if (_stream)
    {
        *_stream << levelToString(level) << getDateTime() << " " << message << std::endl;
    }
}

inline std::string Log::Logger::levelToString(Level level)
{
    switch (level)
    {
    case TRACE:
        return "[ TRACE ]";
    case DEBUG:
        return "[ DEBUG ]";
    case INFO:
        return "[ INFO  ]";
    case WARN:
        return "[ WARN  ]";
    case ERROR:
        return "[ ERROR ]";
    case FATAL:
        return "[ FATAL ]";
    default:
        return "";
    }
}

inline std::string Log::Logger::getDateTime()
{
    const auto now = std::chrono::system_clock::now();
    return "[ " + std::format("{:%d-%m-%Y %H:%M:%OS}", now) + " ]";
}

inline Log::Message::~Message()
{
    if (_level >= _logger._level)
    {
        _logger.output(_level, _stream.str());
    }
}

template<typename T>
inline Log::Message& Log::Message::operator<<(const T& value)
{
    if (_level >= _logger._level)
    {
        _stream << value;
    }
    return *this;
}

inline Log::Message::Message(Logger& logger, Level& level) :
    _logger(logger), _level(level)
{
}

