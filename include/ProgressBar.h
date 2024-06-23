#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>


class ProgressBar
{
public:
    ProgressBar(const std::string& text, int barWidth); //TODO add stream as param
    ~ProgressBar();

public:
    void setNbSteps(int nbSteps);
    void addSteps(int steps);
    void threadExecution();

private:
    bool writeProgress();

private:
    const std::string _text;
    const int _barWidth;
    std::atomic<int> _nbSteps;
    std::atomic<int> _currStep;
    std::mutex _mutex;
};


inline ProgressBar::ProgressBar(const std::string& text, int barWidth) :
    _text(text), _barWidth(barWidth), _nbSteps(0), _currStep(0)
{
}

inline ProgressBar::~ProgressBar()
{
}

inline void ProgressBar::setNbSteps(int nbSteps)
{
    _nbSteps = nbSteps;
}

inline void ProgressBar::addSteps(int steps)
{
    _currStep += steps;
}

inline void ProgressBar::threadExecution()
{
    bool finished = false;
    while (!finished)
    {
        finished = writeProgress();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
}

inline bool ProgressBar::writeProgress()
{
    const std::lock_guard<std::mutex> lock(_mutex);

    if (_currStep > _nbSteps)
        return true;

    float progress = (float)_currStep / (float)_nbSteps;
    int completed = (int)(progress * (float)_barWidth);

    std::cout << "\r" << std::flush;
    std::cout << _text << " [";

    for (int i = 0; i < completed; i++)
        std::cout << "#";
    for (int i = completed; i < _barWidth; i++)
        std::cout << " ";

    std::cout << "] " << (int)(progress * 100.) << "%";

    return _currStep == _nbSteps;
}