#pragma once

#include <chrono>
#include <string>

///
/// \brief Measures scope duration and logs elapsed seconds on destruction.
///
class ElapsedTimer
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

public:
   ///
   /// \brief Starts a timer named "timer".
   ///
   ElapsedTimer();

   ///
   /// \brief Starts a timer with a custom label for log output.
   /// \param name Label printed when the timer is destroyed.
   ///
   ElapsedTimer(const std::string& name);

   ///
   /// \brief Logs elapsed time since construction to standard output.
   ///
   ~ElapsedTimer();

   HighResTimePoint _start_time;
   std::string _name{"timer"};
};
