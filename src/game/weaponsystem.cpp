#include "weaponsystem.h"

#include "weapon.h"


//----------------------------------------------------------------------------------------------------------------------
void WeaponSystem::initialize()
{
   // set up default weapon
   mSelected = std::make_shared<Weapon>();
}
