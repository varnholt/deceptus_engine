#include "elapsedtimer.h"

#include <iomanip>
#include <iostream>
#include <utility>

ElapsedTimer::ElapsedTimer() : _start_time{std::chrono::high_resolution_clock::now()}
{
}

ElapsedTimer::ElapsedTimer(std::string  name) : _start_time{std::chrono::high_resolution_clock::now()}, _name(std::move(name))
{
}

ElapsedTimer::~ElapsedTimer()
{
   const auto now = std::chrono::high_resolution_clock::now();
   const FloatSeconds elapsed = now - _start_time;

   std::cout << _name << " elapsed: " << std::fixed << std::setprecision(9) << elapsed.count() << " seconds" << std::endl;
}
