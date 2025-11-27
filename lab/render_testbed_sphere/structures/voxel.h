#pragma once

#include <cstdint>


struct Voxel
{
   Voxel() = default;

   float mR = 0.0f;
   float mG = 0.0f;
   float mB = 0.0f;
   float mA = 1.0f;

   float mX = 0.0f;
   float mY = 0.0f;
   float mZ = 0.0f;

   uint32_t mGridX = 0;
   uint32_t mGridY = 0;
   uint32_t mGridZ = 0;

   float mSize = 1.0f;

   // indices

   uint32_t mPosAIndex = 0;
   uint32_t mPosBIndex = 0;
   uint32_t mPosCIndex = 0;
   uint32_t mPosDIndex = 0;

   uint32_t mPosEIndex = 0;
   uint32_t mPosFIndex = 0;
   uint32_t mPosGIndex = 0;
   uint32_t mPosHIndex = 0;

   uint32_t mUvAIndex = 0;
   uint32_t mUvBIndex = 0;
   uint32_t mUvCIndex = 0;
   uint32_t mUvDIndex = 0;

   uint32_t mUvEIndex = 0;
   uint32_t mUvFIndex = 0;
   uint32_t mUvGIndex = 0;
   uint32_t mUvHIndex = 0;

   uint32_t mNormalTopIndex = 0;
   uint32_t mNormalBottomIndex = 0;
   uint32_t mNormalLeftIndex = 0;
   uint32_t mNormalRightIndex = 0;
   uint32_t mNormalFrontIndex = 0;
   uint32_t mNormalBackIndex = 0;
};

