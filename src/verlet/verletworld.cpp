// base
#include "verletworld.h"

// verlet
#include "verletconstants.h"
#include "verletedge.h"
#include "verletobject.h"
#include "verletparticle.h"


#include <QLineF>


// https://github.com/SFML/SFML/wiki/Source%3A-Simple-Collision-Detection
// https://github.com/SFML/SFML/wiki/Source:-Simple-Collision-Detection-for-SFML-2
// http://ludobloom.com/tutorials/collision.html


VerletWorld::VerletWorld()
{
}


VerletWorld::~VerletWorld()
{
}


void VerletWorld::updateParticles()
{
   VerletObject* object = 0;

   std::list<VerletObject*>::const_iterator objectIterator;
   for (objectIterator = mObjects.begin(); objectIterator != mObjects.end(); ++objectIterator)
   {
      object = *objectIterator;

      std::list<VerletParticle*>* particles = object->getParticles();
      VerletParticle* particle = 0;

      std::list<VerletParticle*>::const_iterator particleIterator;
      for (particleIterator = particles->begin(); particleIterator != particles->end(); ++particleIterator)
      {
         particle = *particleIterator;

         if (!particle->isPositionFixed())
         {
            sf::Vector3f* current = particle->getPosition();
            sf::Vector3f* previous = particle->getPositionPrevious();

            float vx = (current->x - previous->x) * object->getFriction();
            float vy = (current->y - previous->y) * object->getFriction();

            *previous = *current;

            current->x += vx;
            current->y += vy;
            current->y += GRAVITY;
         }
      }
   }
}


void VerletWorld::constrainParticles()
{
   float width = 3600;
   float height = 3200;

   VerletObject* object = 0;

   std::list<VerletObject*>::const_iterator objectIterator;
   for (objectIterator = mObjects.begin(); objectIterator != mObjects.end(); ++objectIterator)
   {
      object = *objectIterator;

      std::list<VerletParticle*>* particles = object->getParticles();
      VerletParticle* particle = 0;
      float bounce = object->getBouncyness();
      float friction = object->getFriction();

      std::list<VerletParticle*>::const_iterator particleIterator;
      for (particleIterator = particles->begin(); particleIterator != particles->end(); ++particleIterator)
      {
         particle = *particleIterator;

         if (!particle->isPositionFixed())
         {
            sf::Vector3f* current = particle->getPosition();
            sf::Vector3f* previous = particle->getPositionPrevious();

            float vx = (current->x - previous->x) * friction;
            float vy = (current->y - previous->y) * friction;

/*
            // apply bounding box (screen)
            if (current->x > width)
            {
               current->x = width;
               previous->x = current->x + vx * bounce;
            }
            else if (current->x < 0)
            {
               current->x = 0;
               previous->x = current->x + vx * bounce;
            }

            if (current->y > height)
            {
               current->y = height;
               previous->y = current->y + vy * bounce;
            }
            else if (current->y < 0)
            {
               current->y = 0;
               previous->y = current->y + vy * bounce;
            }
*/


            // collision detection with static objects
//            QPointF point(current->x, current->y);
            std::list<QPolygonF>::const_iterator polyIterator;
            QPolygonF poly;
            for (polyIterator = mStaticObjects.begin(); polyIterator != mStaticObjects.end(); ++polyIterator)
            {
               poly = *polyIterator;

//               if (poly.containsPoint(point, Qt::OddEvenFill))
//               {
                  // calc intersection point

                  QPointF intersectionPoint;
                  QLineF l1;
                  l1.setP1(QPointF(current->x, current->y));
                  l1.setP2(QPointF(previous->x, previous->y));

                  for (int pIndex = 0; pIndex < poly.size(); pIndex++)
                  {
                     QLineF l2;
                     l2.setP1(poly.at(pIndex));

                     if (pIndex == poly.size() - 1)
                        l2.setP2(poly.first());
                     else
                        l2.setP2(poly.at(pIndex + 1));

                     if (l1.intersect(l2, &intersectionPoint) == QLineF::BoundedIntersection)
                     {
                        float x = current->x;
                        float y = current->y;

                        do
                        {
                           x -= (current->x - previous->x) * 0.1f;
                           y -= (current->y - previous->y) * 0.1f;
                        }
                        while (poly.containsPoint(QPointF(x, y), Qt::OddEvenFill));

                        current->x = x;
                        current->y = y;

                        previous->x = x; //current->x + vx * bounce;
                        previous->y = y; // current->y + vy * bounce;

                        break;
                     }
                  }

//                  if (poly.containsPoint(QPointF(current->x, previous->y), Qt::OddEvenFill))
//                  {
//                     current->x = previous->x;
//                  }

//                  if (poly.containsPoint(QPointF(previous->x, current->y), Qt::OddEvenFill))
//                  {
//                     current->y = previous->y;
//                  }

//                   current->x = previous->x;
//                   current->y = previous->y;
               }
//            }
         }
      }
   }
}


void VerletWorld::updateEdges()
{
   VerletObject* object = 0;

   std::list<VerletObject*>::const_iterator objectIterator;
   for (objectIterator = mObjects.begin(); objectIterator != mObjects.end(); ++objectIterator)
   {
      object = *objectIterator;

      std::list<VerletEdge*>* edges = object->getEdges();

      std::list<VerletEdge*>::const_iterator edgeIterator;
      for (edgeIterator = edges->begin(); edgeIterator != edges->end(); ++edgeIterator)
      {
         VerletEdge* edge = *edgeIterator;

         sf::Vector3f* p0 = edge->getP0()->getPosition();
         sf::Vector3f* p1 = edge->getP1()->getPosition();

         float dx = p1->x - p0->x;
         float dy = p1->y - p0->y;

         float distance = sqrt(dx * dx + dy * dy);
         float difference = edge->getLength() - distance;
         float ratio = difference / distance / 2.0f;

         float offsetX = dx * ratio;
         float offsetY = dy * ratio;

         if (!edge->getP0()->isPositionFixed())
         {
            p0->x -= offsetX;
            p0->y -= offsetY;
         }

         if (!edge->getP1()->isPositionFixed())
         {
            p1->x += offsetX;
            p1->y += offsetY;
         }
      }
   }
}


std::list<QPolygonF> VerletWorld::getStaticObjects() const
{
   return mStaticObjects;
}


void VerletWorld::setStaticObjects(const std::list<QPolygonF> &staticObjects)
{
   mStaticObjects = staticObjects;
}



void VerletWorld::update(float /*dt*/)
{
   updateParticles();

   for (int i = 0; i < 5; i++)
   {
      updateEdges();
      constrainParticles();
   }
}


void VerletWorld::addObject(VerletObject *object)
{
   mObjects.push_back(object);
}


std::list<VerletObject *> *VerletWorld::getObjects()
{
   return &mObjects;
}


