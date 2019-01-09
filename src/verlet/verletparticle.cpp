#include "verletparticle.h"

VerletParticle::VerletParticle()
 : mPositionFixed(false)
{
}


VerletParticle::VerletParticle(sf::Vector3f pos)
 : mPositionFixed(false),
   mPosition(pos),
   mPositionPrevious(pos)
{
}


sf::Vector3f* VerletParticle::getPosition()
{
   return &mPosition;
}


void VerletParticle::setPosition(const sf::Vector3f &pos)
{
   mPosition = pos;
}


sf::Vector3f *VerletParticle::getPositionPrevious()
{
   return &mPositionPrevious;
}


void VerletParticle::setPositionPrevious(const sf::Vector3f &posPrev)
{
   mPositionPrevious = posPrev;
}


bool VerletParticle::isPositionFixed() const
{
   return mPositionFixed;
}


void VerletParticle::setPositionFixed(bool positionFixed)
{
   mPositionFixed = positionFixed;
}


void VerletParticle::moveBy(float dx, float dy)
{
   mPosition.x += dx;
   mPosition.y += dy;
}



