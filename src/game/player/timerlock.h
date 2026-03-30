#pragma once

#include <chrono>

/// \brief provides a lock state that can auto-release after a delay.
struct TimerLock
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief locks immediately and schedules an unlock once the interval has elapsed.
   /// \param interval duration to keep the lock active before timer-based unlock.
   void lockFor(std::chrono::milliseconds interval);

   /// \brief marks the lock as active.
   void lock();

   /// \brief marks the lock as inactive.
   void unlock();

   /// \brief reports whether the lock is currently active.
   /// \return true when lock() or lockFor() has set the lock and it has not yet been released.
   bool isLocked();

   bool _locked = false;
   TimerLock::HighResTimePoint _unlock_time_point;

};  // namespace MoveLock
