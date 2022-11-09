#include "timerlock.h"
#include "framework/tools/timer.h"

#include <iostream>
#include <mutex>

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
      [this]()
      {
         const auto now = std::chrono::high_resolution_clock::now();
         if (now >= _unlock_time_point)
         {
            unlock();
         }
      }
   );
}
