#include "game/player/weaponsystem.h"
#include "game/weapons/weapon.h"
#include "game/weapons/weaponfactory.h"
#include "game/weapons/bow.h"

#include <algorithm>

namespace
{
void configureWeaponForPlayer(const std::shared_ptr<Weapon>& weapon, b2Body* player_body)
{
   if (!weapon || !player_body)
   {
      return;
   }

   if (auto bow = std::dynamic_pointer_cast<Bow>(weapon))
   {
      bow->setLauncherBody(player_body);
   }
}
}  // namespace

void WeaponSystem::onInventoryItemAdded(const std::string& item_name, b2Body* player_body)
{
   auto existing = std::find_if(
      _weapons.begin(),
      _weapons.end(),
      [&item_name](const auto& weapon) { return weapon && weapon->getName() == item_name; }
   );
   if (existing != _weapons.end())
   {
      configureWeaponForPlayer(*existing, player_body);
      return;
   }

   auto weapon = WeaponFactory::create(item_name);
   if (!weapon)
   {
      return;
   }

   configureWeaponForPlayer(weapon, player_body);
   _weapons.push_back(weapon);
}

void WeaponSystem::onInventoryItemRemoved(const std::string& item_name)
{
   const auto was_selected = _selected && _selected->getName() == item_name;

   _weapons.erase(
      std::remove_if(
         _weapons.begin(),
         _weapons.end(),
         [&item_name](const auto& weapon) { return weapon && weapon->getName() == item_name; }
      ),
      _weapons.end()
   );

   if (was_selected)
   {
      _selected = nullptr;
   }
}

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
      configureWeaponForPlayer(_selected, player_body);
      return;  // Already equipped
   }

   auto selected = std::find_if(
      _weapons.begin(),
      _weapons.end(),
      [&slot_0](const auto& weapon) { return weapon && weapon->getName() == slot_0; }
   );
   if (selected != _weapons.end())
   {
      configureWeaponForPlayer(*selected, player_body);
      _selected = *selected;
      return;
   }

   _selected = nullptr;
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
