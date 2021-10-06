#pragma once

#include <chrono>


class ElapsedTimer
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

public:

   ElapsedTimer();
   ~ElapsedTimer();

   HighResTimePoint _start_time;
};

