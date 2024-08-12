#pragma once

#include <SFML/System/Clock.hpp>

class GlobalClock
{
public:
   GlobalClock();

   static GlobalClock& getInstance();

   int getElapsedTimeInMs();
   float getElapsedTimeInS();
   sf::Time getElapsedTime();

private:
   sf::Clock _clock;
};
