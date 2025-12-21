#pragma once

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <vector>
#include "../glm/glm.hpp"


struct Voxel;

class VboVoxelMesh : public Drawable
{

public:

   enum NormalIndex
   {
      NormalIndexUp    = 0,
      NormalIndexDown  = 1,
      NormalIndexLeft  = 2,
      NormalIndexRight = 3,
      NormalIndexBack  = 4,
      NormalIndexFront = 5
   };

   enum UvIndex
   {
      UvUpLeft    = 0,
      UvUpRight   = 1,
      UvDownLeft  = 2,
      UvDownRight = 3
   };

   struct Vertex
   {
      uint32_t pIndex;
      uint32_t nIndex;
      uint32_t tcIndex;
      uint32_t cIndex;
      uint32_t uvIndex;
   };

   VboVoxelMesh(Voxel** space, uint32_t lengthY, uint32_t lengthZ);
   VboVoxelMesh(Voxel** space, uint32_t lengthX, uint32_t lengthY, uint32_t lengthZ);
   VboVoxelMesh(
      Voxel** space,
      uint32_t slice_from,
      uint32_t slice_count,
      uint32_t lengthX,
      uint32_t lengthY,
      uint32_t lengthZ
   );

   void render() const override;

   Voxel** slice() const;

   uint32_t getLengthY() const;
   uint32_t getLengthZ() const;


protected:

   void setupNormals();
   void setupUVs();
   void addCube(Voxel* current);
   void setupVbo();


   void addCubeFace(
      uint32_t idxa,
      uint32_t idxb,
      uint32_t idxc,
      uint32_t idxd,
      uint32_t cIndexA,
      uint32_t cIndexB,
      uint32_t cIndexC,
      uint32_t cIndexD,
      uint32_t indexNormal
   );

   Voxel** mSpace = nullptr;

   uint32_t mLengthX = 0;
   uint32_t mLengthY = 0;
   uint32_t mLengthZ = 0;

   GLuint mVaoHandle = 0;

   std::vector<glm::vec3> mPositions;
   std::vector<glm::vec3> mNormals;
   std::vector<glm::vec4> mColors;
   std::vector<glm::vec2> mUvs;
   std::vector<GLuint> mFaces;
   std::vector<Vertex> mVertices;
   std::vector<glm::vec3> mVoxelCenters;
};

