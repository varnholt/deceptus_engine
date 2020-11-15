#pragma once

#include <cstdint>

class JumpPlatform
{
public:
   JumpPlatform();
   void touch();
   void dissolve();

   int32_t jumpsLeft = 1;
};

