#include "verletrenderer.h"


#include "verletedge.h"
#include "verletobject.h"
#include "verletparticle.h"

#include <QGLWidget>


VerletRenderer::VerletRenderer()
{
}


void VerletRenderer::renderObjects(const QList<VerletObject *> *objects)
{
   VerletParticle* p = 0;
   VerletEdge* e = 0;

   foreach (VerletObject* object, *objects)
   {
      QList<VerletParticle*>* particles = object->getParticles();
      QList<VerletEdge*>* edges = object->getEdges();

      // edges
      glLineWidth(1.0f);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      glBegin(GL_LINES);
      for (int i = 0; i < edges->length(); i++)
      {
         e = edges->at(i);

         if (e->isVisible())
         {
            glVertex2f(
               e->getP0()->getPosition()->x,
               e->getP0()->getPosition()->y
            );
            glVertex2f(
               e->getP1()->getPosition()->x,
               e->getP1()->getPosition()->y
            );
         }
      }
      glEnd();

      /*
      // particles
      glPointSize(10.0f);
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
      glBegin(GL_POINTS);
      for (int i = 0; i < particles->length(); i++)
      {
         p = particles->at(i);

         glVertex2f(
            p->getPosition()->x,
            p->getPosition()->y
         );
      }
      glEnd();
      */
   }
}


