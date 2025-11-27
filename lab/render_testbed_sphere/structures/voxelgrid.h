#pragma once

#include <string>
#include <vector>

struct Voxel;

class VoxelGrid
{
   enum class ImageFormat
   {
      Rgba,
      Rgb,
   };

   public:

      VoxelGrid();

      virtual ~VoxelGrid();

      static VoxelGrid* getInstance();

      void loadSlices();

      uint32_t getLengthX() const;
      void setLengthX(uint32_t getLengthX);

      uint32_t getLengthY() const;
      void setLengthY(uint32_t getLengthY);

      uint32_t getLengthZ() const;
      void setLengthZ(uint32_t getLengthZ);

      Voxel** makeSlice(uint32_t x);

      void ambientOcclusion(
         uint32_t x, uint32_t y, uint32_t z,
         float &fA, float &fB, float &fC, float &fD,
         float &bE, float &bH, float &bG, float &bF,
         float &rD, float &rC, float &rG, float &rH,
         float &lE, float &lF, float &lB, float &lA,
         float &uE, float &uA, float &uD, float &uH,
         float &dB, float &dF, float &dG, float &dC
      );

   private:

      void imageToVoxelSlice(
         unsigned char* imgData,
         ImageFormat format,
         uint32_t imageHeight,
         uint32_t imageWidth,
         uint32_t xOffset,
         float scale,
         uint32_t threshold = 255
      );

      void removeSurroundedVoxels();
      bool isSurrounded(uint32_t x, uint32_t y, uint32_t z);
      uint32_t getIndex(uint32_t x, uint32_t y, uint32_t z);
      Voxel *getVoxel(uint32_t x, uint32_t y, uint32_t z);

      std::vector<std::string> getSlicesNames();

      uint32_t mLengthX;
      uint32_t mLengthY;
      uint32_t mLengthZ;

      Voxel** mGrid;

      static VoxelGrid* sInstance;
};

