#ifndef VERLETRENDERER_H
#define VERLETRENDERER_H

#include <QList>

class VerletObject;


class VerletRenderer
{
   public:

      VerletRenderer();

      void renderObjects(const QList<VerletObject*>* objects);
};

#endif // VERLETRENDERER_H
