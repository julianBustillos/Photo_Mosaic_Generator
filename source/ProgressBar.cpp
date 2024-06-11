#include "ProgressBar.h"
#include <iostream>
#include <thread>


ProgressBar::ProgressBar(const std::string& text, int barWidth) :
    _text(text), _barWidth(barWidth), _nbSteps(0), _currStep(0)
{
}

ProgressBar::~ProgressBar()
{
}

void ProgressBar::setNbSteps(int nbSteps)
{
    _nbSteps = nbSteps;
}

void ProgressBar::addSteps(int steps)
{
    _currStep += steps;
}

void ProgressBar::threadExecution()
{
    bool finished = false;
    while (!finished)
    {
        finished = writeProgress();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool ProgressBar::writeProgress()
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
