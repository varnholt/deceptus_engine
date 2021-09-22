#pragma once

#include <memory>
#include <vector>

#include <SFML/Graphics.hpp>

class Weapon;

struct WeaponSystem
{
   WeaponSystem() = default;
   void initialize();

   std::shared_ptr<Weapon> _selected;
   std::vector<std::shared_ptr<Weapon>> _weapons;
};

