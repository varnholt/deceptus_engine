#include "gamemechanism.h"


int32_t GameMechanism::getZ() const
{
   return mZ;
}


void GameMechanism::setZ(const int32_t& z)
{
   mZ = z;
}


bool GameMechanism::getEnabled() const
{
   return mEnabled;
}


void GameMechanism::setEnabled(bool enabled)
{
   mEnabled = enabled;
}
