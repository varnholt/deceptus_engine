#include "game/player/weaponsystem.h"
#include "game/weapons/weapon.h"
#include "game/weapons/weaponfactory.h"

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
