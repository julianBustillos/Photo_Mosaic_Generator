#pragma once

#include <chrono>
#include <iostream>


#define TIME_NOW(time) std::chrono::high_resolution_clock::time_point time = std::chrono::high_resolution_clock::now();
#define PRINT_DURATION(start, end) int count = (int)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); int min = count / 60000; int sec = (count - min * 60000) / 1000; int ms = count - min * 60000 - sec * 1000; std::cout << "Execution time : " << min << "min " << sec << "s " << ms << "ms" << std::endl << std::endl;
