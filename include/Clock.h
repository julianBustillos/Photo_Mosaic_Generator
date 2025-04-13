#pragma once

#include <chrono>
#include "termcolor.hpp"


class Clock
{
public:
    Clock() 
    { 
        _startTime = std::chrono::high_resolution_clock::now(); 
    };
    ~Clock() {};
    std::string getTimeStamp()
    {
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        int count = (int)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - _startTime).count();
        int min = count / 60000; 
        int sec = (count - min * 60000) / 1000; 
        int ms = count - min * 60000 - sec * 1000;
        std::string timeStamp = "Execution time : " + std::to_string(min) + "min " + std::to_string(sec) + "s " + std::to_string(ms) + "ms";
        return timeStamp;
    }

private:
    std::chrono::high_resolution_clock::time_point _startTime;
};