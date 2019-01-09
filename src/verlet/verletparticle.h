#ifndef VERLETPARTICLE_H
#define VERLETPARTICLE_H

#include <SFML/System/Vector3.hpp>


class VerletParticle
{

public:

   VerletParticle();
   VerletParticle(sf::Vector3f pos);

   sf::Vector3f* getPosition();
   void setPosition(const sf::Vector3f &pos);

   sf::Vector3f* getPositionPrevious();
   void setPositionPrevious(const sf::Vector3f &posPrev);

   bool isPositionFixed() const;
   void setPositionFixed(bool positionFixed);

   void moveBy(float dx, float dy);


protected:

   sf::Vector3f mPosition;
   sf::Vector3f mPositionPrevious;
   bool mPositionFixed;
};

#endif // VERLETPARTICLE_H
