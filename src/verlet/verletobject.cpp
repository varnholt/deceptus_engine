#include "verletobject.h"


VerletObject::VerletObject()
 : mFriction(0.999f),
   mBouncyness(0.9f),
   mStiffness(1)
{
}


std::list<VerletParticle *>* VerletObject::getParticles()
{
   return &mParticles;
}


void VerletObject::setParticles(const std::list<VerletParticle *> &particles)
{
   mParticles = particles;
}


void VerletObject::addParticle(VerletParticle *particle)
{
   mParticles.push_back(particle);
}


std::list<VerletEdge *>* VerletObject::getEdges()
{
   return &mEdges;
}


void VerletObject::setEdges(const std::list<VerletEdge *> &edges)
{
   mEdges = edges;
}


void VerletObject::addEdge(VerletEdge *edge)
{
   mEdges.push_back(edge);
}


float VerletObject::getFriction() const
{
   return mFriction;
}


void VerletObject::setFriction(float friction)
{
   mFriction = friction;
}


float VerletObject::getBouncyness() const
{
   return mBouncyness;
}


void VerletObject::setBouncyness(float bouncyness)
{
   mBouncyness = bouncyness;
}


int VerletObject::getStiffness() const
{
   return mStiffness;
}


void VerletObject::setStiffness(int stiffness)
{
   mStiffness = stiffness;
}





