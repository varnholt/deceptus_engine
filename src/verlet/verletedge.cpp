#include "verletedge.h"

// verlet
#include "verletparticle.h"

// sfml
#include <SFML/System/Vector3.hpp>

// cmath
#include <math.h>


VerletEdge::VerletEdge(VerletParticle *p0, VerletParticle *p1)
 : mLength(1.0f),
   mVisible(true)
{
   mParticles[0]=p0;
   mParticles[1]=p1;
   mLength = computeDistance();
}


float VerletEdge::getLength() const
{
   return mLength;
}


void VerletEdge::setLength(float len)
{
   mLength = len;
}


float VerletEdge::computeDistance()
{
   VerletParticle* p0 = mParticles[0];
   VerletParticle* p1 = mParticles[1];
   float dx = p1->getPosition()->x - p0->getPosition()->x;
   float dy = p1->getPosition()->y - p0->getPosition()->y;
   float distance = sqrt(dx * dx + dy * dy);
   return distance;
}


bool VerletEdge::isVisible() const
{
   return mVisible;
}


void VerletEdge::setVisible(bool visible)
{
   mVisible = visible;
}


VerletParticle *VerletEdge::getP0()
{
   return mParticles[0];
}


VerletParticle *VerletEdge::getP1()
{
   return mParticles[1];
}


