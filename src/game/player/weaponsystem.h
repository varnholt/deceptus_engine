#pragma once

#include <memory>
#include <vector>
#include <array>
#include <string>

#include "json/json.hpp"

class Weapon;
struct b2Body;

struct WeaponSystem
{
   WeaponSystem() = default;

   void onInventoryItemAdded(const std::string& item_name, b2Body* player_body);
   void onInventoryItemRemoved(const std::string& item_name);
   void syncWithInventory(const std::array<std::string, 2>& slots, b2Body* player_body);

   std::shared_ptr<Weapon> _selected;
   std::vector<std::shared_ptr<Weapon>> _weapons;
};

void to_json(nlohmann::json& j, const WeaponSystem& d);
void from_json(const nlohmann::json& j, WeaponSystem& d);
