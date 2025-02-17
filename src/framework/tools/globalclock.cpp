#include "globalclock.h"

#include <SFML/System/Time.hpp>

GlobalClock::GlobalClock()
{
   _clock.restart();
}

GlobalClock& GlobalClock::getInstance()
{
   static GlobalClock __instance;
   return __instance;
}

int GlobalClock::getElapsedTimeInMs()
{
   return _clock.getElapsedTime().asMilliseconds();
}

float GlobalClock::getElapsedTimeInS()
{
   return _clock.getElapsedTime().asMilliseconds() * 0.001f;
}

sf::Time GlobalClock::getElapsedTime()
{
   return _clock.getElapsedTime();
}
