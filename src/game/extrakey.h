#ifndef EXTRAKEY_H
#define EXTRAKEY_H

#include "extra.h"

struct ExtraKey : public Extra
{
   enum Key
   {
      Green    = 0x01,
      Yellow   = 0x02,
      Red      = 0x04,
      KeySkull = 0x08,
      KeyMoon  = 0x10,
      KeySun   = 0x20
   };

   ExtraKey();

   int mKeys = 0;
};

#endif // EXTRAKEY_H
