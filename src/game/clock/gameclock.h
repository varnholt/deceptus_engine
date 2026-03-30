#pragma once

#include <chrono>

/// \brief tracks elapsed high-resolution time since the game clock was last reset.
class GameClock
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

public:
   /// \brief returns the global game clock instance.
   /// \return singleton clock used for runtime timing queries.
   static GameClock& getInstance();

   /// \brief stores the current high-resolution timestamp as the new start time.
   void reset();

   /// \brief returns the elapsed duration since the most recent reset().
   /// \return high-resolution duration between now and the stored start time.
   HighResDuration durationSinceSpawn() const;

private:
   /// \brief constructs the singleton clock without an initialized start timestamp.
   GameClock() = default;
   HighResTimePoint _start_time;
};
