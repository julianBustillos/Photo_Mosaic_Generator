#pragma once

#include "ProgressBar.h"
#include "termcolor.h"
#include <iostream>
#include <thread>
#include <mutex>


namespace Console
{
    enum Type
    {
        DEFAULT,
        HELP,
        TIME,
        ERROR
    };

    class Message;

    class Out
    {
        friend class Message;

    public:
        static Message get(Type type);
        static void initBar(const std::string& text, int nbSteps);
        static void addBarSteps(int steps);
        static void startBar(Type type);
        static void waitBar();

    private:
        Out();
        ~Out();
        Message createMessage(Type type);
        void output(Type type, const std::string& message);

    private:
        static Out& get();
        static void SetColor(Type type);

    private:
        std::mutex _mutex;
        ProgressBar _progressBar;
        std::thread* _thread;
    };

    class Message
    {
        friend class Out;

    public:
        ~Message();
        template <typename T>
        Message& operator<<(const T& value);

    private:
        Message(Out& out, Type type);

    private:
        Out& _out;
        const Type _type;
        std::stringstream _stream;
    };
}


inline Console::Message Console::Out::get(Type type)
{
    return get().createMessage(type);
}

inline void Console::Out::initBar(const std::string& text, int nbSteps)
{
    get()._progressBar.initialize(text, 70, nbSteps);
}

inline void Console::Out::addBarSteps(int steps)
{
    get()._progressBar.addSteps(steps);
}

inline void Console::Out::startBar(Type type)
{
    if (!get()._thread)
    {
        get()._thread = new std::thread(&ProgressBar::threadUpdate, &get()._progressBar);
    }
}

inline void Console::Out::waitBar()
{
    if (get()._thread)
    {
        get()._thread->join();
        delete get()._thread;
        get()._thread = nullptr;
    }
}

inline Console::Out::Out() :
    _progressBar(std::cout), _thread(nullptr)
{
}

inline Console::Out::~Out()
{
    std::cout << termcolor::reset;
}

inline Console::Message Console::Out::createMessage(Type type)
{
    return Message(*this, type);
}

inline void Console::Out::output(Type type, const std::string& message)
{
    if (!_thread)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        SetColor(type);
        std::cout << message << std::endl;
    }
}

inline Console::Out& Console::Out::get()
{
    static Out out;
    return out;
}

inline void Console::Out::SetColor(Type type)
{
    switch (type)
    {
    case HELP:
        std::cout << termcolor::bright_yellow;
        break;
    case TIME:
        std::cout << termcolor::bright_blue;
        break;
    case ERROR:
        std::cout << termcolor::bright_red;
        break;
    default:
        std::cout << termcolor::bright_green;
        break;
    }
}

inline Console::Message::~Message()
{
    _out.output(_type, _stream.str());
}

template <typename T>
inline Console::Message& Console::Message::operator<<(const T& value)
{
    _stream << value;
    return *this;
}

inline Console::Message::Message(Out& out, Type type) :
    _out(out), _type(type)
{
}

