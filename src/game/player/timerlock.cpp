#include "framework/tools/timer.h"
#include "timerlock.h"

#include <iostream>
#include <mutex>

namespace
{
bool _locked = false;
TimerLock::HighResTimePoint _unlock_time_point;
}  // namespace

void TimerLock::lock()
{
   _locked = true;
}

void TimerLock::unlock()
{
   _locked = false;
}

bool TimerLock::isLocked()
{
   return _locked;
}

void TimerLock::lockFor(std::chrono::milliseconds interval)
{
   // bump time to unlock
   lock();
   const auto now = std::chrono::high_resolution_clock::now();
   _unlock_time_point = now + interval;

   Timer::add(
      interval,
      []()
      {
         const auto now = std::chrono::high_resolution_clock::now();
         if (now >= _unlock_time_point)
         {
            unlock();
         }
      }
   );
}
