#ifndef WORLD_H
#define WORLD_H

// std
#include <list>

// sfml
#include <SFML/System/Vector3.hpp>

// Qt
#include <QPolygonF>


class VerletObject;


class VerletWorld
{
   public:

      VerletWorld();
      ~VerletWorld();

      void update(float dt);


      std::list<VerletObject*>* getObjects();
      void addObject(VerletObject* object);


      std::list<QPolygonF> getStaticObjects() const;
      void setStaticObjects(const std::list<QPolygonF> &objects);


protected:

      void updateParticles();
      void constrainParticles();
      void updateEdges();


      // forces

      sf::Vector3f mGravity;
      sf::Vector3f mWind;


      std::list<QPolygonF> mStaticObjects;


      std::list<VerletObject*> mObjects;
};

#endif // WORLD_H
