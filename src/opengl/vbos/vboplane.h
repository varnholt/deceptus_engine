#pragma once

#include "interfaces/drawable.h"

/// \brief subdivided xz-plane mesh with normals and texture coordinates.
class VBOPlane : public Drawable
{
private:
   unsigned int vaoHandle;
   int faces;

public:
   /// \brief generates a tessellated plane and uploads it to OpenGL buffers.
   /// \param xsize total size along x axis.
   /// \param zsize total size along z axis.
   /// \param xdivs number of subdivisions along x axis.
   /// \param zdivs number of subdivisions along z axis.
   /// \param smax maximum s texture coordinate.
   /// \param tmax maximum t texture coordinate.
   VBOPlane(float xsize, float zsize, int xdivs, int zdivs, float smax = 1.0f, float tmax = 1.0f);

   /// \brief draws the plane as indexed triangles.
   void render() const override;
};
