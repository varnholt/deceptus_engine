#ifndef VBOCUBE_H
#define VBOCUBE_H

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

/// \brief static unit cube geometry backed by a VAO and indexed triangles.
class VBOCube : public Drawable
{

private:
    unsigned int vaoHandle;

public:
   /// \brief builds cube vertex, normal, texcoord and index buffers.
   VBOCube();

   /// \brief draws the cube geometry as indexed triangles.
   void render() const override;
};

#endif  // VBOCUBE_H
