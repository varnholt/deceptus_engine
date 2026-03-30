#pragma once

#include <cstdint>

#include "game/weapons/projectile.h"

/// \brief projectile specialization used by the bow weapon.
struct Arrow : public Projectile
{
   /// \brief configures arrow-specific projectile behavior and shared hit resources.
   Arrow();

   /// \brief tracks arrow destruction and reports suspicious lifetime leaks.
   ~Arrow();
   int32_t _start_time = 0;
   static bool _animation_initialised;
};
