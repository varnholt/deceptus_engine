#pragma once

#include <chrono>

struct TimerLock
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   void lockFor(std::chrono::milliseconds interval);
   void lock();
   void unlock();
   bool isLocked();

   bool _locked = false;
   TimerLock::HighResTimePoint _unlock_time_point;

};  // namespace MoveLock
