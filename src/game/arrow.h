#pragma once

#include <cstdint>

#include "projectile.h"


struct Arrow : public Projectile
{
   Arrow();
   void updateTextureRect();
   int32_t _start_time = 0;
   static bool _animation_initialised;
};
