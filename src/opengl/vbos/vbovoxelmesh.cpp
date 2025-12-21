#include "vbovoxelmesh.h"

#include "structures/voxel.h"
#include "structures/voxelgrid.h"

#include <iostream>


VboVoxelMesh::VboVoxelMesh(Voxel** space, uint32_t lengthY, uint32_t lengthZ)
 : mSpace(space),
   mLengthY(lengthY),
   mLengthZ(lengthZ)
{
   setupNormals();
   setupUVs();

   Voxel* current = nullptr;
   for (auto y = 0u; y < lengthY; y++)
   {
      for (auto z = 0u; z < lengthZ; z++)
      {
         current = mSpace[y * lengthY + z];
         if (current)
         {
            addCube(current);
         }
      }
   }

   setupVbo();
}


VboVoxelMesh::VboVoxelMesh(Voxel** space, uint32_t lengthX, uint32_t lengthY, uint32_t lengthZ)
 : mSpace(space),
   mLengthX(lengthX),
   mLengthY(lengthY),
   mLengthZ(lengthZ)
{
   setupNormals();
   setupUVs();

   Voxel* current = nullptr;
   for (auto z = 0u; z < lengthZ; z++)
   {
      for (auto y = 0u; y < lengthY; y++)
      {
         for (auto x = 0u; x < lengthX; x++)
         {
            current = mSpace[
                     z * lengthX * lengthY
                   + y * lengthX
                   + x
               ];

            if (current)
            {
               addCube(current);
            }
         }
      }
   }

   setupVbo();
}


VboVoxelMesh::VboVoxelMesh(
   Voxel** space,
   uint32_t slice_from,
   uint32_t slice_count,
   uint32_t lengthX,
   uint32_t lengthY,
   uint32_t lengthZ
)
 : mSpace(space),
   mLengthX(lengthX),
   mLengthY(lengthY),
   mLengthZ(lengthZ)
{
   setupNormals();
   setupUVs();

   Voxel* current = nullptr;
   for (auto z = 0u; z < lengthZ; z++)
   {
      for (auto y = 0u; y < lengthY; y++)
      {
         for (auto x = slice_from; x < slice_from + slice_count; x++)
         {
            current = mSpace[
                     z * lengthX * lengthY
                   + y * lengthX
                   + x
               ];

            if (current)
            {
               addCube(current);
            }
         }
      }
   }

   setupVbo();
}


void VboVoxelMesh::setupNormals()
{
   // 6 default normals
   glm::vec3 nUp    = glm::vec3( 0, 1, 0);
   glm::vec3 nDown  = glm::vec3( 0,-1, 0);
   glm::vec3 nLeft  = glm::vec3(-1, 0, 0);
   glm::vec3 nRight = glm::vec3( 1, 0, 0);
   glm::vec3 nBack  = glm::vec3( 0, 0, 1);
   glm::vec3 nFront = glm::vec3( 0, 0,-1);

   mNormals.push_back(nUp);
   mNormals.push_back(nDown);
   mNormals.push_back(nLeft);
   mNormals.push_back(nRight);
   mNormals.push_back(nBack);
   mNormals.push_back(nFront);
}


void VboVoxelMesh::setupUVs()
{
   // 4 default uvs
   glm::vec2 uvUpLeft    = glm::vec2(0,0);
   glm::vec2 uvUpRight   = glm::vec2(1,0);
   glm::vec2 uvDownLeft  = glm::vec2(0,1);
   glm::vec2 uvDownRight = glm::vec2(1,1);

   mUvs.push_back(uvUpLeft);
   mUvs.push_back(uvUpRight);
   mUvs.push_back(uvDownLeft);
   mUvs.push_back(uvDownRight);
}


void VboVoxelMesh::setupVbo()
{
   glm::vec3 center;
   for (auto c : mVoxelCenters)
   {
      center += c;
   }

   center /= static_cast<float>(mVoxelCenters.size());

   float* positions = new float[3 * mVertices.size()];
   float* normals   = new float[3 * mVertices.size()];
   float* colors    = new float[4 * mVertices.size()];
   float* uvs       = new float[2 * mVertices.size()];
   float* sCenters  = new float[3 * mVertices.size()];
   float* vCenters  = new float[3 * mVertices.size()];

   auto voxelIndex = 0u;
   for (auto i = 0u; i < mVertices.size(); i++)
   {
      positions[i * 3    ] = mPositions[mVertices[i].pIndex].x;
      positions[i * 3 + 1] = mPositions[mVertices[i].pIndex].y;
      positions[i * 3 + 2] = mPositions[mVertices[i].pIndex].z;

      normals[i * 3    ] = mNormals[mVertices[i].nIndex].x;
      normals[i * 3 + 1] = mNormals[mVertices[i].nIndex].y;
      normals[i * 3 + 2] = mNormals[mVertices[i].nIndex].z;

      colors[i * 4    ] = mColors[mVertices[i].cIndex].x;
      colors[i * 4 + 1] = mColors[mVertices[i].cIndex].y;
      colors[i * 4 + 2] = mColors[mVertices[i].cIndex].z;
      colors[i * 4 + 3] = mColors[mVertices[i].cIndex].w;

      uvs[i * 2    ] = mUvs[mVertices[i].uvIndex].x;
      uvs[i * 2 + 1] = mUvs[mVertices[i].uvIndex].y;

      sCenters[i * 3    ] = center.x;
      sCenters[i * 3 + 1] = center.y;
      sCenters[i * 3 + 2] = center.z;

      vCenters[i * 3    ] = mVoxelCenters[voxelIndex].x;
      vCenters[i * 3 + 1] = mVoxelCenters[voxelIndex].y;
      vCenters[i * 3 + 2] = mVoxelCenters[voxelIndex].z;

      // if (((i >> 3) << 3) == i)
      if (i > 0 && (i % 24) == 0)
      {
         voxelIndex++;
      }
   }

   // create and populate the buffer objects
   unsigned int handle[7];
   glGenBuffers(7, handle);

   glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
   glBufferData(GL_ARRAY_BUFFER, (3 * mVertices.size()) * sizeof(float), positions, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
   glBufferData(GL_ARRAY_BUFFER, (3 * mVertices.size()) * sizeof(float), normals, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, handle[2]);
   glBufferData(GL_ARRAY_BUFFER, (4 * mVertices.size()) * sizeof(float), colors, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, handle[3]);
   glBufferData(GL_ARRAY_BUFFER, (2 * mVertices.size()) * sizeof(float), uvs, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, handle[4]);
   glBufferData(GL_ARRAY_BUFFER, (3 * mVertices.size()) * sizeof(float), sCenters, GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, handle[5]);
   glBufferData(GL_ARRAY_BUFFER, (3 * mVertices.size()) * sizeof(float), vCenters, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[6]);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, mFaces.size() * sizeof(unsigned int), &mFaces[0], GL_STATIC_DRAW);

   delete[] positions;
   delete[] normals;
   delete[] colors;
   delete[] uvs;
   delete[] sCenters;
   delete[] vCenters;

   // create VAO
   glGenVertexArrays(1, &mVaoHandle);
   glBindVertexArray(mVaoHandle);

   glEnableVertexAttribArray(0);  // position
   glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

   glEnableVertexAttribArray(1);  // normal
   glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

   glEnableVertexAttribArray(2);  // color
   glBindBuffer(GL_ARRAY_BUFFER, handle[2]);
   glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

   glEnableVertexAttribArray(3);  // uv
   glBindBuffer(GL_ARRAY_BUFFER, handle[3]);
   glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

   glEnableVertexAttribArray(4);  // slice center
   glBindBuffer(GL_ARRAY_BUFFER, handle[4]);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

   glEnableVertexAttribArray(5);  // voxel center
   glBindBuffer(GL_ARRAY_BUFFER, handle[5]);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[6]);
   glBindVertexArray(0);
}


void VboVoxelMesh::addCube(Voxel* voxel)
{
   glm::vec3 base(voxel->mX, voxel->mY, voxel->mZ);
   auto size = voxel->mSize;

   auto posA = base + glm::vec3(0,    size,  0   );
   auto posB = base + glm::vec3(0,    0,     0   );
   auto posC = base + glm::vec3(0,    0,    -size);
   auto posD = base + glm::vec3(0,    size, -size);
   auto posE = base + glm::vec3(size, size,  0   );
   auto posF = base + glm::vec3(size, 0,     0   );
   auto posG = base + glm::vec3(size, 0,    -size);
   auto posH = base + glm::vec3(size, size, -size);

   auto centerVoxel = base + glm::vec3(size * 0.5f, size * 0.5f, -size * 0.5f);
   mVoxelCenters.push_back(centerVoxel);

   float cfa = 1.0f; float cfb = 1.0f; float cfc = 1.0f; float cfd = 1.0f;
   float cbe = 1.0f; float cbh = 1.0f; float cbg = 1.0f; float cbf = 1.0f;
   float crd = 1.0f; float crc = 1.0f; float crg = 1.0f; float crh = 1.0f;
   float cle = 1.0f; float clf = 1.0f; float clb = 1.0f; float cla = 1.0f;
   float cue = 1.0f; float cua = 1.0f; float cud = 1.0f; float cuh = 1.0f;
   float cdb = 1.0f; float cdf = 1.0f; float cdg = 1.0f; float cdc = 1.0f;

   VoxelGrid* grid = VoxelGrid::getInstance();
   grid->ambientOcclusion(
      voxel->mGridX, voxel->mGridY, voxel->mGridZ,
      cfa, cfb, cfc, cfd,
      cbe, cbh, cbg, cbf,
      crd, crc, crg, crh,
      cle, clf, clb, cla,
      cue, cua, cud, cuh,
      cdb, cdf, cdg, cdc
   );

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cfa);
   auto cIndexFA = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cfb);
   auto cIndexFB = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cfc);
   auto cIndexFC = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cfd);
   auto cIndexFD = static_cast<uint32_t>(mColors.size()) - 1;

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cbe);
   auto cIndexBE = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cbh);
   auto cIndexBH = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cbg);
   auto cIndexBG = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cbf);
   auto cIndexBF = static_cast<uint32_t>(mColors.size()) - 1;

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * crd);
   auto cIndexRD = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * crc);
   auto cIndexRC = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * crg);
   auto cIndexRG = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * crh);
   auto cIndexRH = static_cast<uint32_t>(mColors.size()) - 1;

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cle);
   auto cIndexLE = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * clf);
   auto cIndexLF = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * clb);
   auto cIndexLB = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cla);
   auto cIndexLA = static_cast<uint32_t>(mColors.size()) - 1;

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cue);
   auto cIndexUE = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cua);
   auto cIndexUA = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cud);
   auto cIndexUD = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cuh);
   auto cIndexUH = static_cast<uint32_t>(mColors.size()) - 1;

   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cdb);
   auto cIndexDB = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cdf);
   auto cIndexDF = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cdg);
   auto cIndexDG = static_cast<uint32_t>(mColors.size()) - 1;
   mColors.push_back(glm::vec4(voxel->mR, voxel->mG, voxel->mB, voxel->mA) * cdc);
   auto cIndexDC = static_cast<uint32_t>(mColors.size()) - 1;

   mPositions.push_back(posA);
   auto idxA = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posB);
   auto idxB = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posC);
   auto idxC = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posD);
   auto idxD = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posE);
   auto idxE = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posF);
   auto idxF = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posG);
   auto idxG = static_cast<uint32_t>(mPositions.size()) - 1;

   mPositions.push_back(posH);
   auto idxH = static_cast<uint32_t>(mPositions.size()) - 1;

   //           1,1,-1
   //     e-------h
   //    /|      /|
   //   a-------d |
   //   | |     | |
   //   | f_____|_g
   //   |/      |/
   //   b-------c
   // 0,0,0
   //
   // faces:
   //
   //   front
   //      a,b,c
   //      a,c,d
   //   back
   //      e,g,f
   //      e,h,g
   //   right
   //      d,c,g
   //      d,g,h
   //   left
   //      e,f,b
   //      e,b,a
   //   top
   //      e,a,d
   //      e,d,h
   //   bottom
   //      f,c,b
   //      f,g,c

   // no need to render bottom and back side

   addCubeFace(idxA, idxB, idxC, idxD, cIndexFB, cIndexFC, cIndexFD, cIndexFA, static_cast<uint32_t>(NormalIndexFront));
   addCubeFace(idxE, idxH, idxG, idxF, cIndexBG, cIndexBF, cIndexBE, cIndexBH, static_cast<uint32_t>(NormalIndexBack));
   addCubeFace(idxD, idxC, idxG, idxH, cIndexRH, cIndexRD, cIndexRC, cIndexRG, static_cast<uint32_t>(NormalIndexRight));
   addCubeFace(idxE, idxF, idxB, idxA, cIndexLB, cIndexLA, cIndexLE, cIndexLF, static_cast<uint32_t>(NormalIndexLeft));
   addCubeFace(idxE, idxA, idxD, idxH, cIndexUH, cIndexUE, cIndexUA, cIndexUD, static_cast<uint32_t>(NormalIndexUp));
   addCubeFace(idxB, idxF, idxG, idxC, cIndexDF, cIndexDG, cIndexDC, cIndexDB, static_cast<uint32_t>(NormalIndexDown));
}


void VboVoxelMesh::addCubeFace(
   uint32_t idxa,
   uint32_t idxb,
   uint32_t idxc,
   uint32_t idxd,
   uint32_t cIndexA,
   uint32_t cIndexB,
   uint32_t cIndexC,
   uint32_t cIndexD,
   uint32_t nIndex
)
{
   Vertex a,b,c,d;

   a.pIndex = idxa;
   b.pIndex = idxb;
   c.pIndex = idxc;
   d.pIndex = idxd;

   a.nIndex = nIndex;
   b.nIndex = nIndex;
   c.nIndex = nIndex;
   d.nIndex = nIndex;

   a.cIndex = cIndexA;
   b.cIndex = cIndexB;
   c.cIndex = cIndexC;
   d.cIndex = cIndexD;

   //     e-------h
   //    /|      /|
   //   a-------d |
   //   | |     | |
   //   | f_____|_g
   //   |/      |/
   //   b-------c

   a.uvIndex = UvUpLeft;
   b.uvIndex = UvDownLeft;
   c.uvIndex = UvDownRight;
   d.uvIndex = UvUpRight;

   uint32_t vidx1, vidx2, vidx3, vidx4;

   mVertices.push_back(a);
   vidx1 = static_cast<uint32_t>(mVertices.size()) - 1;

   mVertices.push_back(b);
   vidx2 = static_cast<uint32_t>(mVertices.size()) - 1;

   mVertices.push_back(c);
   vidx3 = static_cast<uint32_t>(mVertices.size()) - 1;

   mVertices.push_back(d);
   vidx4 = static_cast<uint32_t>(mVertices.size()) - 1;

   mFaces.push_back(vidx1);
   mFaces.push_back(vidx2);
   mFaces.push_back(vidx3);

   mFaces.push_back(vidx1);
   mFaces.push_back(vidx3);
   mFaces.push_back(vidx4);
}


void VboVoxelMesh::render() const
{
   // std::cout << "rendering " << mFaces.size() << " faces" << std::endl;

   glBindVertexArray(mVaoHandle);
   glDrawElements(GL_TRIANGLES, mFaces.size(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
}


Voxel** VboVoxelMesh::slice() const
{
   return mSpace;
}


uint32_t VboVoxelMesh::getLengthZ() const
{
   return mLengthZ;
}


uint32_t VboVoxelMesh::getLengthY() const
{
   return mLengthY;
}


