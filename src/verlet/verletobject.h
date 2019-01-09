#ifndef VERLETOBJECT_H
#define VERLETOBJECT_H

// std
#include <list>

class VerletEdge;
class VerletParticle;


class VerletObject
{

public:


   VerletObject();

   std::list<VerletParticle *>* getParticles();
   void setParticles(const std::list<VerletParticle *> &particle);
   void addParticle(VerletParticle* particle);

   std::list<VerletEdge *>* getEdges();
   void setEdges(const std::list<VerletEdge *> &edges);
   void addEdge(VerletEdge* edge);

   float getFriction() const;
   void setFriction(float friction);

   float getBouncyness() const;
   void setBouncyness(float bouncyness);


   int getStiffness() const;
   void setStiffness(int stiffness);



protected:


   std::list<VerletParticle*> mParticles;
   std::list<VerletEdge*> mEdges;

   float mFriction;
   float mBouncyness;
   int mStiffness;

};

#endif // VERLETOBJECT_H
