#pragma once

#include "opengl/interfaces/drawable.h"
#include "opengl/gl_current.h"

class VBOSphere : public Drawable
{

   public:

      VBOSphere(float, GLuint, GLuint);
      void render() const;

      int getVertexArrayHandle();

   private:

      void generateVerts(float * , float * ,float *, GLuint *);

      unsigned int vaoHandle;
      GLuint nVerts, elements;
      float radius;
      GLuint slices, stacks;

};

