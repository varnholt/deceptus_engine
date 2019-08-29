#pragma once

#include "extra.h"

struct ExtraKey : public Extra
{
   enum Key
   {
      Green    = 0x01,
      Yellow   = 0x02,
      Red      = 0x04,
      Blue     = 0x08,
      KeySkull = 0x10,
      KeyMoon  = 0x20,
      KeySun   = 0x40
   };

   ExtraKey();

   int mKeys = 0;
};

