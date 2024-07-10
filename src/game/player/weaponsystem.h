#pragma once

#include <memory>
#include <vector>

#include "json/json.hpp"

class Weapon;

struct WeaponSystem
{
   WeaponSystem() = default;

   std::shared_ptr<Weapon> _selected;
   std::vector<std::shared_ptr<Weapon>> _weapons;
};

void to_json(nlohmann::json& j, const WeaponSystem& d);
void from_json(const nlohmann::json& j, WeaponSystem& d);
