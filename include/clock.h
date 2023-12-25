#pragma once

#include <chrono>
#include <iostream>


class Clock
{
public:
    Clock() {};
    ~Clock() {};
    void start() 
    { 
        _startTime = std::chrono::high_resolution_clock::now();
    };
    void end()
    {
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        int count = (int)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - _startTime).count();
        int min = count / 60000; 
        int sec = (count - min * 60000) / 1000; 
        int ms = count - min * 60000 - sec * 1000;
        std::cout << "Execution time : " << min << "min " << sec << "s " << ms << "ms" << std::endl << std::endl;
    }

private:
    std::chrono::high_resolution_clock::time_point _startTime;
};