#include "menuconfig.h"

#include <fstream>
#include "json/json.hpp"

MenuConfig::MenuConfig()
{
   std::ifstream file("data/config/menus.json");
   nlohmann::json j;
   file >> j;

   _duration_hide = FloatSeconds(j["duration_hide"].get<float>());
   _duration_show = FloatSeconds(j["duration_show"].get<float>());

   // the section is optional, so the defaults of InventoryLayout survive a file that predates it
   const auto inventory_it = j.find("inventory");
   if (inventory_it == j.end())
   {
      return;
   }

   const auto& inventory = *inventory_it;

   const auto read_positions = [&inventory](const char* key, std::array<float, 2>& target)
   {
      const auto found = inventory.find(key);
      if (found == inventory.end() || !found->is_array() || found->size() != target.size())
      {
         return;
      }
      for (auto index = size_t{0}; index < target.size(); index++)
      {
         target[index] = (*found)[index].get<float>();
      }
   };

   const auto read_position = [&inventory](const char* key, float& target)
   {
      const auto found = inventory.find(key);
      if (found == inventory.end() || !found->is_number())
      {
         return;
      }
      target = found->get<float>();
   };

   read_positions("slot_badge_x_px", _inventory._slot_badge_x_px);
   read_position("slot_badge_y_px", _inventory._slot_badge_y_px);
   read_positions("equip_hint_x_px", _inventory._equip_hint_x_px);
   read_position("equip_hint_y_px", _inventory._equip_hint_y_px);
}
