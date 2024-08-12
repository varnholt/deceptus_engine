#pragma once

#include <chrono>
#include <string>

class ElapsedTimer
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

public:
   ElapsedTimer();
   ElapsedTimer(const std::string& name);
   ~ElapsedTimer();

   HighResTimePoint _start_time;
   std::string _name{"timer"};
};
