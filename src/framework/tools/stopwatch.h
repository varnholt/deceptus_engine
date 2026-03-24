#pragma once

#include <chrono>

///
/// \brief Measures high-resolution elapsed durations from a shared start point.
///
class StopWatch
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

public:
   ///
   /// \brief Returns the global stopwatch singleton.
   /// \return Stopwatch singleton.
   ///
   static StopWatch& getInstance();

   ///
   /// \brief Resets the internal start time to now.
   ///
   void reset();

   ///
   /// \brief Returns elapsed time since the last reset.
   /// \return Elapsed duration from internal start time to now.
   ///
   HighResDuration duration() const;
   ///
   /// \brief Returns elapsed time from the last reset to `time_point`.
   /// \param time_point End time point.
   /// \return Elapsed duration.
   ///
   HighResDuration duration(const HighResTimePoint& time_point);

   ///
   /// \brief Returns elapsed time between two time points.
   /// \param earlier Start time point.
   /// \param later End time point.
   /// \return Duration between `earlier` and `later`.
   ///
   static HighResDuration duration(const HighResTimePoint& earlier, const HighResTimePoint& later);
   ///
   /// \brief Returns current high-resolution time point.
   /// \return Current time point.
   ///
   static HighResTimePoint now();

private:
   StopWatch() = default;
   HighResTimePoint _start_time;
};
