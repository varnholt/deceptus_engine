#include "gameclock.h"


GameClock& GameClock::getInstance()
{
   static GameClock __instance;
   return __instance;
}


void GameClock::reset()
{
   _start_time = std::chrono::high_resolution_clock::now();
}


GameClock::HighResDuration GameClock::duration() const
{
   const auto now = std::chrono::high_resolution_clock::now();
   return now - _start_time;
}

