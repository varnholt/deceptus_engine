#pragma once

#include <cstdint>
#include <vector>


struct Physics
{
   uint32_t mGridWidth = 0;
   uint32_t mGridHeight = 0;
   uint32_t mGridSize = 0;

   std::vector<int32_t> mPhysicsMap;
};

