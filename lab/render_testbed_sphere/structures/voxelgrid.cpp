#include "voxelgrid.h"

#include "voxel.h"

#ifdef WIN32
#include <Windows.h>
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>


VoxelGrid* VoxelGrid::sInstance = nullptr;


VoxelGrid::VoxelGrid()
 : mLengthX(0),
   mLengthY(0),
   mLengthZ(0),
   mGrid(nullptr)
{
   sInstance = this;
}


VoxelGrid::~VoxelGrid()
{
   delete[] mGrid;
}


VoxelGrid *VoxelGrid::getInstance()
{
   if (!sInstance)
      new VoxelGrid();

   return sInstance;
}


// Now data should contain the (R, G, B) values of the pixels.
// The color of pixel (i, j) is stored at
//    data[j * width + i],
//    data[j * width + i + 1] and
//    data[j * width + i + 2].
unsigned char* readBmp(const std::string& filename, uint32_t& width, uint32_t& height)
{
   // read the 54-byte header
   int i;
   FILE* file = fopen(filename.c_str(), "rb");
   unsigned char info[54];
   fread(info, sizeof(unsigned char), 54, file);

   // extract image height and width from header
   width = *(uint32_t*)&info[18];
   height = *(uint32_t*)&info[22];

   // allocate 3 bytes per pixel and read the rest of the data at once
   int size = 3 * width * height;
   unsigned char* data = new unsigned char[size];
   fread(data, sizeof(unsigned char), size, file);
   fclose(file);

   // bgr to rgb
   for (i = 0; i < size; i += 3)
   {
      unsigned char tmp = data[i];

      data[i]     = data[i + 2];
      data[i + 2] = tmp;
   }

   return data;
}


void VoxelGrid::imageToVoxelSlice(
    unsigned char* imgData,
    ImageFormat format,
    uint32_t lengthY,
    uint32_t lengthZ,
    uint32_t xOffset,
    float scale,
    uint32_t threshold
)
{
   uint8_t r = 0;
   uint8_t g = 0;
   uint8_t b = 0;
   uint8_t a = 0;

   uint8_t bpp = (format == ImageFormat::Rgba) ? 4 : 3;

   for (auto imageY = 0u; imageY < lengthY; imageY++)
   {
      for (auto imageX = 0u; imageX < lengthZ; imageX++)
      {
         auto imagePos = (imageY * lengthZ + imageX) * bpp;

         r = imgData[imagePos    ];
         g = imgData[imagePos + 1];
         b = imgData[imagePos + 2];

         if (format == ImageFormat::Rgba)
         {
            a = imgData[imagePos + 3];
         }
         else
         {
            a = 255;
         }

         if (r <= threshold)
         {
            Voxel* v = new Voxel();

            v->mGridX = xOffset;
            v->mGridY = imageY;
            v->mGridZ = (mLengthZ - 1) - imageX;

            v->mX = xOffset * scale;
            v->mY = imageY * scale;
            v->mZ = ((mLengthZ - 1) - imageX) * scale;

            v->mR = r / 255.0f;
            v->mG = g / 255.0f;
            v->mB = b / 255.0f;
            v->mA = a / 255.0f;

            v->mSize = scale;

            auto index = getIndex(v->mGridX, v->mGridY, v->mGridZ);
            mGrid[index]=v;
         }
      }
   }
}


void VoxelGrid::removeSurroundedVoxels()
{
   std::vector<Voxel*> surroundedVoxels;
   for (auto slice = 0u; slice < mLengthX; slice++)
   {
      for (auto imageY = 0u; imageY < mLengthY; imageY++)
      {
         for (auto imageX = 0u; imageX < mLengthZ; imageX++)
         {
            auto index = getIndex(slice, imageY, imageX);
            Voxel* voxel = mGrid[index];

            if (voxel)
            {
               if (isSurrounded(slice, imageY, imageX))
               {

                  // printf("delete voxel: %d,%d,%d\n", imageX, imageY, slice);
                  surroundedVoxels.push_back(voxel);
               }
            }
         }
      }
   }

   for (auto v : surroundedVoxels)
   {
      auto index = getIndex(v->mGridX, v->mGridY, v->mGridZ);
      mGrid[index] = nullptr;
      // delete v;
   }
   // printf("file: %s, w: %d, h: %d\n", entry.c_str(), imageWidth, imageHeight);
}


std::vector<std::string> VoxelGrid::getSlicesNames()
{
   std::vector<std::string> slices;
   std::string path = "tunnel_volume_slices";

   for (auto& p: std::filesystem::directory_iterator(path))
   {
      if (!std::filesystem::is_regular_file(p))
         continue;

      std::filesystem::path filePath{p};
      std::string fileName = filePath.string();

      if (fileName.find("tunnel") != std::string::npos)
      {
         slices.push_back(fileName);
      }
   }

   return slices;
}


void VoxelGrid::loadSlices()
{
   // tunnel0000.bmp
   // tunnel0768.bmp

   float scale = 1.0f;
   uint8_t threshhold = 230;

   auto slices = getSlicesNames();

   mLengthX = slices.size();

   auto sliceNumber = 0u;
   for (const auto& entry : slices)
   {
      unsigned char* imgData = readBmp(entry, mLengthZ, mLengthY);

      if (!mGrid)
      {
         auto size = mLengthX * mLengthY * mLengthZ;

         mGrid = new Voxel*[size];

         for (auto i = 0u; i < size; i++)
         {
            mGrid[i] = nullptr;
         }
      }

      imageToVoxelSlice(
         imgData,
         ImageFormat::Rgb,
         mLengthY,
         mLengthZ,
         sliceNumber,
         scale,
         threshhold
      );

      delete[] imgData;
      sliceNumber++;
   }

   // cleanup surrounded voxels
   removeSurroundedVoxels();

   printf("VoxelGrid::loadSlices(): loaded %d slices\n", mLengthX);
}


uint32_t VoxelGrid::getLengthX() const
{
   return mLengthX;
}


void VoxelGrid::setLengthX(uint32_t length)
{
   mLengthX = length;
}


uint32_t VoxelGrid::getLengthY() const
{
   return mLengthY;
}


void VoxelGrid::setLengthY(uint32_t length)
{
   mLengthY = length;
}


uint32_t VoxelGrid::getLengthZ() const
{
   return mLengthZ;
}


void VoxelGrid::setLengthZ(uint32_t length)
{
   mLengthZ = length;
}


Voxel** VoxelGrid::makeSlice(uint32_t sliceNo)
{
   Voxel** slice = new Voxel*[mLengthZ * mLengthY];

   for (auto imageY = 0u; imageY < mLengthY; imageY++)
   {
      for (auto imageX = 0u; imageX < mLengthZ; imageX++)
      {
         auto index2d = (mLengthZ * imageY) + imageX;
         auto index3d = getIndex(sliceNo, imageY, imageX);

         slice[index2d] = mGrid[index3d];
      }
   }

   return slice;
}


bool VoxelGrid::isSurrounded(uint32_t x, uint32_t y, uint32_t z)
{
   auto a = getVoxel(x - 1, y,     z    );
   auto b = getVoxel(x + 1, y,     z    );
   auto c = getVoxel(x,     y - 1, z    );
   auto d = getVoxel(x,     y + 1, z    );
   auto e = getVoxel(x,     y,     z - 1);
   auto f = getVoxel(x,     y,     z + 1);

   bool surrounded = (a && b && c && d && e && f);
   return surrounded;
}


Voxel* VoxelGrid::getVoxel(uint32_t x, uint32_t y, uint32_t z)
{
   Voxel* v = nullptr;

   if (
         x < mLengthX
      && y < mLengthY
      && z < mLengthZ
   )
   {
      auto index = getIndex(x, y, z);
      v = mGrid[index];
   }

   return v;
}


uint32_t VoxelGrid::getIndex(uint32_t x, uint32_t y, uint32_t z)
{
   uint32_t index = 0;
   index = (((z * mLengthY) + y) * mLengthX) + x;
   return index;
}


// addCubeFace(idxA, idxB, idxC, idxD, cIndexFA, cIndexFB, cIndexFC, cIndexFD, (int)NormalIndexFront);
// addCubeFace(idxE, idxH, idxG, idxF, cIndexBE, cIndexBH, cIndexBG, cIndexBF, (int)NormalIndexBack);
// addCubeFace(idxD, idxC, idxG, idxH, cIndexRD, cIndexRC, cIndexRG, cIndexRH, (int)NormalIndexRight);
// addCubeFace(idxE, idxF, idxB, idxA, cIndexLE, cIndexLF, cIndexLB, cIndexLA, (int)NormalIndexLeft);
// addCubeFace(idxE, idxA, idxD, idxH, cIndexUE, cIndexUA, cIndexUD, cIndexUH, (int)NormalIndexUp);
// addCubeFace(idxB, idxF, idxG, idxC, cIndexDB, cIndexDF, cIndexDG, cIndexDC, (int)NormalIndexDown);

void VoxelGrid::ambientOcclusion(
   uint32_t x, uint32_t y, uint32_t z,
   float& fa, float& fb, float& fc, float& fd,
   float& be, float& bh, float& bg, float& bf,
   float& rd, float& rc, float& rg, float& rh,
   float& le, float& lf, float& lb, float& la,
   float& ue, float& ua, float& ud, float& uh,
   float& db, float& df, float& dg, float& dc
)
{
   // float factor = 0.125f;
   float factor = 0.2f;

   float x1y1z1 = ((getVoxel(x - 1, y - 1, z - 1) == nullptr) ? 0.0f : factor);
   float x2y1z1 = ((getVoxel(x    , y - 1, z - 1) == nullptr) ? 0.0f : factor);
   float x3y1z1 = ((getVoxel(x + 1, y - 1, z - 1) == nullptr) ? 0.0f : factor);
   float x1y1z2 = ((getVoxel(x - 1, y - 1, z    ) == nullptr) ? 0.0f : factor);
   // float x2y1z2 = ((getVoxel(x    , y - 1, z    ) == nullptr) ? 0.0f : factor); // not needed
   float x3y1z2 = ((getVoxel(x + 1, y - 1, z    ) == nullptr) ? 0.0f : factor);
   float x1y1z3 = ((getVoxel(x - 1, y - 1, z + 1) == nullptr) ? 0.0f : factor);
   float x2y1z3 = ((getVoxel(x    , y - 1, z + 1) == nullptr) ? 0.0f : factor);
   float x3y1z3 = ((getVoxel(x + 1, y - 1, z + 1) == nullptr) ? 0.0f : factor);

   float x1y2z1 = ((getVoxel(x - 1, y,     z - 1) == nullptr) ? 0.0f : factor);
   float x2y2z1 = ((getVoxel(x    , y,     z - 1) == nullptr) ? 0.0f : factor);
   float x3y2z1 = ((getVoxel(x + 1, y,     z - 1) == nullptr) ? 0.0f : factor);
   // float x1y2z2 = ((getVoxel(x - 1, y,     z    ) == nullptr) ? 0.0f : factor); // not needed
   // float x2y2z2 = ((getVoxel(x    , y,     z    ) == nullptr) ? 0.0f : factor); // self, not needed
   // float x3y2z2 = ((getVoxel(x + 1, y,     z    ) == nullptr) ? 0.0f : factor); // not needed
   float x1y2z3 = ((getVoxel(x - 1, y,     z + 1) == nullptr) ? 0.0f : factor);
   // float x2y2z3 = ((getVoxel(x    , y,     z + 1) == nullptr) ? 0.0f : factor); // not needed
   float x3y2z3 = ((getVoxel(x + 1, y,     z + 1) == nullptr) ? 0.0f : factor);

   float x1y3z1 = ((getVoxel(x - 1, y + 1, z - 1) == nullptr) ? 0.0f : factor);
   float x2y3z1 = ((getVoxel(x    , y + 1, z - 1) == nullptr) ? 0.0f : factor);
   float x3y3z1 = ((getVoxel(x + 1, y + 1, z - 1) == nullptr) ? 0.0f : factor);
   float x1y3z2 = ((getVoxel(x - 1, y + 1, z    ) == nullptr) ? 0.0f : factor);
   // float x2y3z2 = ((getVoxel(x    , y + 1, z    ) == nullptr) ? 0.0f : factor); // top center, not needed
   float x3y3z2 = ((getVoxel(x + 1, y + 1, z    ) == nullptr) ? 0.0f : factor);
   float x1y3z3 = ((getVoxel(x - 1, y + 1, z + 1) == nullptr) ? 0.0f : factor);
   float x2y3z3 = ((getVoxel(x    , y + 1, z + 1) == nullptr) ? 0.0f : factor);
   float x3y3z3 = ((getVoxel(x + 1, y + 1, z + 1) == nullptr) ? 0.0f : factor);

   fa -= (x1y3z1 + x1y2z1 + x2y2z1);
   fb -= (x1y2z1 + x1y1z1 + x2y1z1);
   fc -= (x2y1z1 + x3y1z1 + x3y2z1);
   fd -= (x2y3z1 + x3y3z1 + x3y2z1);

   le -= (x1y2z3 + x1y3z2 + x1y3z3);
   la -= (x1y2z1 + x1y3z1 + x1y3z2);
   lf -= (x1y2z3 + x1y1z3 + x1y1z2);
   lb -= (x1y2z1 + x1y1z1 + x1y1z2);

   rd -= (x2y2z1 + x3y2z1 + x3y3z1);
   rc -= (x2y1z1 + x3y1z1 + x3y2z1);
   rg -= (x2y1z3 + x3y1z3 + x3y2z3);
   rh -= (x3y2z3 + x2y3z3 + x3y3z3);

   ue -= (x1y3z2 + x1y3z3 + x2y3z3);
   uh -= (x2y3z3 + x3y3z3 + x3y3z2);
   ud -= (x3y3z2 + x3y3z1 + x2y3z1);
   ua -= (x1y3z2 + x1y3z1 + x2y3z1);

   df -= (x1y1z2 + x1y1z3 + x2y1z3);
   db -= (x1y1z1 + x1y1z2 + x2y1z1);
   dc -= (x2y1z1 + x3y1z1 + x3y1z2);
   dg -= (x2y1z3 + x3y1z3 + x3y1z2);

   be -= (x1y3z3 + x1y2z3 + x2y3z3);
   bf -= (x1y2z3 + x1y1z3 + x2y1z3);
   bh -= (x2y3z3 + x3y3z3 + x3y2z3);
   bg -= (x2y1z3 + x3y1z3 + x3y2z3);
}


/*
   bottom layer: y1
   middle layer: y2
   top layer:    y3

   front layer:  z1
   middle layer: z2
   back layer:   z3

   left layer:   x1
   middle layer: x2
   right layer:  x3


   front:
   a: x1y3z1, x1y2z1, x2y2z1
   b: x1y2z1, x1y1z1, x2y1z1
   c: x2y1z1, x3y1z1, x3y2z1
   d: x2y3z1, x3y3z1, x3y2z1

   right:
   d: x2y2z1 + x3y2z1 + x3y3z1
   c: x2y1z1 + x3y1z1 + x3y2z1
   g: x2y1z3 + x3y1z3 + x3y2z3
   h: x3y2z3 + x2y3z3 + x3y3z3

   bottom:
   f: x1y1z2 + x1y1z3 + x2y1z3
   b: x1y1z1 + x1y1z2 + x2y1z1
   c: x2y1z1 + x3y1z1 + x3y1z2
   g: x2y1z3 + x3y1z3 + x3y1z2

   left:
   a: x1y2z1 + x1y3z1 + x1y3z2
   b: x1y2z1 + x1y1z1 + x1y1z2
   e: x1y2z3 + x1y3z2 + x1y3z3
   f: x1y1z3 + x1y1z3 + x1y1z2

                                 Zp
                                /
       x-------x-------x-------x
      /       /       /       /|
     x-------x-------x-------x |
    /       /|  x+0  |  x+1  | |
   x-------x |  y+1  |  y+1  | x
   |  x-1  | |  z+1  |  z+1  |/|
   |  y+1  | e-------h-------x |
   |  z+1  |/|      /|      /| |
   x-------a-------d-------x | x
   | |     | |     | |     | |/|
   | x_____| f_____|_g_____|_x |
   |/      |/      |/      |/| |
   x-------b-------c-------x | x
   | |     | |     | |     | |/
   | x_____|_x_____|_x_____|_x
   |/      |/      |/      |/
   x-------x-------x-------x---------Xp



   +-------------+-------------+-------------+
   |.............|.............|.............|
   |.............|.............|.............|
   |.............|.............|.............|
   |......A......|......B......|......C......|
   |.............|.............|.............|
   |.............|.............|.............|
   |.............|.............|.............|
   +-------------e-------------h-------------+ e.weight = A+B+D
   |.............|             |.............| h.weight = B+C+E
   |.............|             |.............| d.weight = E+G+H
   |.............|             |.............| a.weight = D+F+G
   |......D......|             |......E......|
   |.............|             |.............|
   |.............|             |.............|
   |.............|             |.............|
   +-------------a-------------d-------------+
   |.............|.............|.............|
   |.............|.............|.............|
   |.............|.............|.............|
   |......F......|......G......|......H......|
   |.............|.............|.............|
   |.............|.............|.............|
   |.............|.............|.............|
   +-------------+-------------+-------------+

*/
