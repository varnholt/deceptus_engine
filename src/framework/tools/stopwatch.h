#pragma once

#include <chrono>

class StopWatch
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

public:

   static StopWatch& getInstance();

   void reset();

   HighResDuration duration() const;
   HighResDuration duration(const HighResTimePoint& time_point);

   static HighResDuration duration(const HighResTimePoint& earlier, const HighResTimePoint& later);
   static HighResTimePoint now();

private:

   StopWatch() = default;
   HighResTimePoint _start_time;
};

