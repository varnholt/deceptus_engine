#ifndef WEAPONSYSTEM_H
#define WEAPONSYSTEM_H

#include <memory>
#include <vector>

#include "weaponitem.h"

struct WeaponSystem
{
   std::shared_ptr<WeaponItem> mSelected;
   std::vector<std::shared_ptr<WeaponItem>> mWeapons;

   WeaponSystem();
};

#endif // WEAPONSYSTEM_H
