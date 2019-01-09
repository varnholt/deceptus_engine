
#ifndef GLOBALCLOCK_H
#define GLOBALCLOCK_H

#include <SFML/System/Clock.hpp>

class GlobalClock
{
public:


   GlobalClock();

   static GlobalClock* getInstance();

   int getElapsedTimeInMs();
   float getElapsedTimeInS();
   sf::Time getElapsedTime();


protected:

   sf::Clock mClock;

   static GlobalClock* sInstance;

};

#endif // GLOBALCLOCK_H
