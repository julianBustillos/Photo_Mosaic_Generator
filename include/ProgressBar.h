#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>


class ProgressBar
{
public:
    ProgressBar(std::ostream& stream);
    ~ProgressBar();

public:
    void initialize(const std::string& text, int width, int nbSteps);
    void addSteps(int steps);
    void threadUpdate();

private:
    bool writeProgress();

private:
    std::ostream& _stream;
    std::string _text;
    int _width;
    std::atomic<int> _nbSteps;
    std::atomic<int> _currStep;
    std::mutex _mutex;
};


inline ProgressBar::ProgressBar(std::ostream& stream) :
    _stream(stream), _text(""), _width(0), _nbSteps(0), _currStep(0)
{
}

inline ProgressBar::~ProgressBar()
{
}

inline void ProgressBar::initialize(const std::string& text, int width, int nbSteps)
{
    _text = text;
    _width = width;
    _nbSteps = nbSteps;
    _currStep = 0;
}

inline void ProgressBar::addSteps(int steps)
{
    _currStep += steps;
}

inline void ProgressBar::threadUpdate()
{
    bool finished = false;
    while (!finished)
    {
        finished = writeProgress();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _stream << std::endl;
}

inline bool ProgressBar::writeProgress()
{
    const std::lock_guard<std::mutex> lock(_mutex);

    if (_currStep > _nbSteps)
        return true;

    float progress = (float)_currStep / (float)_nbSteps;
    int percentage = (int)(progress * 100.);
    std::string padding = percentage < 10 ? "  " : percentage < 100 ? " " : "";
    int completed = (int)(progress * (float)_width);

    _stream << "\r" << std::flush;
    _stream << _text << " [";

    for (int i = 0; i < completed; i++)
        _stream << "#";
    for (int i = completed; i < _width; i++)
        _stream << " ";

    _stream << "] " << padding << percentage << "%";

    return _currStep == _nbSteps;
}