#include "elapsedtimer.h"

#include <iomanip>
#include <iostream>



ElapsedTimer::ElapsedTimer()
{
   _start_time = std::chrono::high_resolution_clock::now();
}


ElapsedTimer::~ElapsedTimer()
{
   const auto now = std::chrono::high_resolution_clock::now();
   const FloatSeconds elapsed = now - _start_time;

   std::cout << "elapsed: " << std::fixed << std::setprecision(9) << elapsed.count() << " seconds" << std::endl;
}
