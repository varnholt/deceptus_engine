#include "game/player/weaponsystem.h"
#include "game/weapons/weapon.h"
#include "game/weapons/weaponfactory.h"
#include "game/weapons/bow.h"

void WeaponSystem::syncWithInventory(const std::array<std::string, 2>& slots, b2Body* player_body)
{
   // For now, the first slot weapon becomes the selected weapon
   const auto& slot_0 = slots[0];

   if (slot_0.empty())
   {
      _selected = nullptr;
      return;
   }

   // Check if we already have this weapon selected
   if (_selected && _selected->getName() == slot_0)
   {
      return;  // Already equipped
   }

   // Try to find existing weapon in _weapons vector
   auto it = std::find_if(
      _weapons.begin(),
      _weapons.end(),
      [&slot_0](const auto& w) { return w && w->getName() == slot_0; }
   );

   if (it != _weapons.end())
   {
      _selected = *it;
      return;
   }

   // Create new weapon from factory
   auto new_weapon = WeaponFactory::create(slot_0);
   if (new_weapon)
   {
      // Special setup for bow - set launcher body
      if (slot_0 == "Bow" && player_body)
      {
         std::dynamic_pointer_cast<Bow>(new_weapon)->setLauncherBody(player_body);
      }
      _weapons.push_back(new_weapon);
      _selected = new_weapon;
   }
}

void to_json(nlohmann::json& j, const WeaponSystem& d)
{
   // for now sufficient to just store their names
   // there's 'WeaponProperties' in case there's a need to serialize more content
   std::vector<std::string> weapon_names;
   std::ranges::transform(d._weapons, std::back_inserter(weapon_names), [](const auto& weapon) { return weapon->getName(); });

   j["weapons"] = weapon_names;

   // store selected
   if (d._selected)
   {
      j["selected"] = d._selected->getName();
   }
}

void from_json(const nlohmann::json& j, WeaponSystem& d)
{
   static const std::unordered_map<std::string, WeaponType> weapon_map = {
      {"bow", WeaponType::Bow}, {"gun", WeaponType::Gun}, {"sword", WeaponType::Sword}
   };

   // code below does not set the launcher body (of the player) for the bow yet
   if (const auto it = j.find("weapons"); it != j.end())
   {
      const auto weapon_names = it->get<std::vector<std::string>>();
      for (const auto& weapon_name : weapon_names)
      {
         if (auto w_it = weapon_map.find(weapon_name); w_it != weapon_map.end())
         {
            d._weapons.emplace_back(WeaponFactory::create(w_it->second));
         }
      }
   }

   if (const auto it = j.find("selected"); it != j.end())
   {
      const auto selected_name = it->get<std::string>();
      if (auto selected_weapon =
             std::ranges::find_if(d._weapons, [&selected_name](const auto& weapon) { return weapon->getName() == selected_name; });
          selected_weapon != d._weapons.end())
      {
         d._selected = *selected_weapon;
      }
   }
}
