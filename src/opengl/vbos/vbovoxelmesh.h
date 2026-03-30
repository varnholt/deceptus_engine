#pragma once

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <vector>
#include "../glm/glm.hpp"


struct Voxel;

/// \brief voxel-based mesh builder that emits cube faces into OpenGL buffers.
class VboVoxelMesh : public Drawable
{
public:

   /// \brief indices for precomputed face normals used by cube faces.
   enum NormalIndex
   {
      NormalIndexUp    = 0,
      NormalIndexDown  = 1,
      NormalIndexLeft  = 2,
      NormalIndexRight = 3,
      NormalIndexBack  = 4,
      NormalIndexFront = 5
   };

   /// \brief indices for canonical quad UV coordinates.
   enum UvIndex
   {
      UvUpLeft    = 0,
      UvUpRight   = 1,
      UvDownLeft  = 2,
      UvDownRight = 3
   };

   /// \brief packed index references for one generated mesh vertex.
   struct Vertex
   {
      uint32_t pIndex;
      uint32_t nIndex;
      uint32_t tcIndex;
      uint32_t cIndex;
      uint32_t uvIndex;
   };

   /// \brief builds a voxel mesh for a 2d slice interpreted over y/z dimensions.
   /// \param space pointer array containing voxel pointers.
   /// \param lengthY number of cells along y.
   /// \param lengthZ number of cells along z.
   VboVoxelMesh(Voxel** space, uint32_t lengthY, uint32_t lengthZ);

   /// \brief builds a voxel mesh for a full 3d voxel field.
   /// \param space pointer array containing voxel pointers.
   /// \param lengthX number of cells along x.
   /// \param lengthY number of cells along y.
   /// \param lengthZ number of cells along z.
   VboVoxelMesh(Voxel** space, uint32_t lengthX, uint32_t lengthY, uint32_t lengthZ);

   /// \brief builds a voxel mesh for a contiguous x-axis slice range.
   /// \param space pointer array containing voxel pointers.
   /// \param slice_from starting x index of included slice.
   /// \param slice_count number of x slices to include.
   /// \param lengthX number of cells along x in source field.
   /// \param lengthY number of cells along y in source field.
   /// \param lengthZ number of cells along z in source field.
   VboVoxelMesh(
      Voxel** space,
      uint32_t slice_from,
      uint32_t slice_count,
      uint32_t lengthX,
      uint32_t lengthY,
      uint32_t lengthZ
   );

   /// \brief draws generated voxel faces as indexed triangles.
   void render() const override;

   /// \brief returns the raw voxel pointer storage used to build this mesh.
   /// \return pointer to voxel pointer array.
   Voxel** slice() const;

   /// \brief returns stored y-dimension length.
   /// \return number of cells along y.
   uint32_t getLengthY() const;

   /// \brief returns stored z-dimension length.
   /// \return number of cells along z.
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

