#pragma once

#include <chrono>
#include <iostream>


#define TIME_NOW(time) std::chrono::high_resolution_clock::time_point time = std::chrono::high_resolution_clock::now();
#define PRINT_DURATION(start, end) std::cout << "Execution time : " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " milliseconds" << std::endl << std::endl;
