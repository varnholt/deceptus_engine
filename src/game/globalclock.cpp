#include "globalclock.h"


GlobalClock* GlobalClock::sInstance = 0;


GlobalClock::GlobalClock()
{
   sInstance = this;
   mClock.restart();
}


GlobalClock *GlobalClock::getInstance()
{
   if (!sInstance)
      new GlobalClock;

   return sInstance;
}


int GlobalClock::getElapsedTimeInMs()
{
   return mClock.getElapsedTime().asMilliseconds();
}


float GlobalClock::getElapsedTimeInS()
{
   return mClock.getElapsedTime().asMilliseconds() * 0.001f;
}


sf::Time GlobalClock::getElapsedTime()
{
   return mClock.getElapsedTime();
}
