#ifndef PLAYERDASH_H
#define PLAYERDASH_H

#include <cstdint>
#include "game/constants.h"

struct PlayerDash
{
   int32_t _dash_frame_count = 0;
   float _dash_multiplier = 0.0f;
   Dash _dash_dir = Dash::None;

   void abort()
   {
      _dash_frame_count = 0;
   }

   bool isDashActive() const
   {
      return (_dash_frame_count > 0);
   }
};

#endif // PLAYERDASH_H
