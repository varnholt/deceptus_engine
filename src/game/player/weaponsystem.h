#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "json/json.hpp"

class Weapon;
struct b2Body;

struct WeaponSystem
{
   WeaponSystem() = default;

   std::shared_ptr<Weapon> _selected;
   std::vector<std::shared_ptr<Weapon>> _weapons;
};

void to_json(nlohmann::json& j, const WeaponSystem& d);
void from_json(const nlohmann::json& j, WeaponSystem& d);
