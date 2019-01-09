#ifndef EXTRAHEALTH_H
#define EXTRAHEALTH_H

#include "extra.h"

struct ExtraHealth : Extra
{
   ExtraHealth();

   void reset();


   void addHalth(int health);

   int mLives = 5;
   int mHealth = 100;
   int mHealthMax = 100;
};

#endif // EXTRAHEALTH_H
