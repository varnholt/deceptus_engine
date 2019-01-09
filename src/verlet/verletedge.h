#ifndef VERLETEDGE_H
#define VERLETEDGE_H

class VerletParticle;

class VerletEdge
{

public:

   VerletEdge(VerletParticle* p0, VerletParticle* p1);


   float getLength() const;
   void setLength(float len);
   float computeDistance();

   bool isVisible() const;
   void setVisible(bool visible);

   VerletParticle* getP0();
   VerletParticle* getP1();

protected:

   VerletParticle* mParticles[2];
   float mLength;
   bool mVisible;
};

#endif // VERLETEDGE_H
