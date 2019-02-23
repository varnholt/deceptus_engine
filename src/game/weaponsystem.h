#ifndef WEAPONSYSTEM_H
#define WEAPONSYSTEM_H

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

class Weapon;

struct WeaponSystem
{
   WeaponSystem() = default;
   void initialize();

   std::shared_ptr<Weapon> mSelected;
   std::vector<std::shared_ptr<Weapon>> mWeapons;
};

#endif // WEAPONSYSTEM_H
