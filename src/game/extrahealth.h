#pragma once

#include "extra.h"

struct ExtraHealth : Extra
{
   ExtraHealth();

   void reset();


   void addHealth(int health);

   int mLives = 5;
   int mHealth = 100;
   int mHealthMax = 100;
};

