#pragma once

#include <chrono>

class GameClock
{
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

public:
   static GameClock& getInstance();

   void reset();
   HighResDuration durationSinceSpawn() const;

private:
   GameClock() = default;
   HighResTimePoint _start_time;
};
