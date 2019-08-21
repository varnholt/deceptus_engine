#include "bumper.h"

#include "game/globalclock.h"


Bumper::Bumper()
{

}


void Bumper::touch()
{
   mStartTime = GlobalClock::getInstance()->getElapsedTime();
}


void Bumper::update(const sf::Time& /*dt*/)
{

}
