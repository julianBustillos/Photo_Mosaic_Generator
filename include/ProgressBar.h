#pragma once

#include <string>
#include <atomic>
#include <mutex>


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

