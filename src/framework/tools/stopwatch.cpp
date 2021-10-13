#include "stopwatch.h"


StopWatch& StopWatch::getInstance()
{
   static StopWatch __instance;
   return __instance;
}


void StopWatch::reset()
{
   _start_time = std::chrono::high_resolution_clock::now();
}


StopWatch::HighResDuration StopWatch::duration() const
{
   const auto now = std::chrono::high_resolution_clock::now();
   return now - _start_time;
}


StopWatch::HighResDuration StopWatch::duration(const StopWatch::HighResTimePoint& time_point)
{
   return time_point - _start_time;
}


StopWatch::HighResDuration StopWatch::duration(const StopWatch::HighResTimePoint& earlier, const StopWatch::HighResTimePoint& later)
{
   return later - earlier;
}


StopWatch::HighResTimePoint StopWatch::now()
{
   return std::chrono::high_resolution_clock::now();
}

