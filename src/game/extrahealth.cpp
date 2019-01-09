#include "extrahealth.h"


ExtraHealth::ExtraHealth()
{
   mExtraType = ExtraType::Health;
}


void ExtraHealth::reset()
{
   mHealth = 100;
}


void ExtraHealth::addHalth(int health)
{
   mHealth += health;
   if (mHealth > mHealthMax)
   {
      mHealth = mHealthMax;
   }
}
