#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/mechanisms/extra.h"
#include "game/mechanisms/infooverlay.h"
#include "game/mechanisms/lever.h"
#include "game/mechanisms/treasurechest.h"

extern "C"
{
#include "lauxlib.h"
#include "lualib.h"
}

#include <iostream>
#include <stdexcept>

namespace
{
void check(bool condition, const char* message)
{
   if (!condition)
   {
      throw std::runtime_error(message);
   }
}

GameDeserializeData make_data(const std::string& name)
{
   GameDeserializeData data;
   data._tmx_object = std::make_shared<TmxObject>();
   data._tmx_object->_name = name;
   data._tmx_object->_properties = std::make_shared<TmxProperties>();
   return data;
}

void set_string(GameDeserializeData& data, const std::string& name, const std::string& value)
{
   auto property = std::make_shared<TmxProperty>();
   property->_value_string = value;
   data._tmx_object->_properties->_map[name] = property;
}

void test_overlay()
{
   auto data = make_data("zone_1");
   set_string(data, "texture", "data/level-catacombs/zone.png");
   auto original = InfoOverlay::setup(nullptr, data);
   nlohmann::json saved;
   original->serializeState(saved);
   check(!saved["zone_1"]["shown"].get<bool>(), "fresh overlay already shown");
   original->setEnabled(true);
   original->serializeState(saved);
   check(saved["zone_1"]["shown"].get<bool>(), "active overlay not saved");
   original->update(sf::seconds(20));
   original->setEnabled(true);
   check(!original->isEnabled(), "completed overlay replayed");

   InfoOverlay restored;
   restored.setObjectId("zone_1");
   restored.deserializeState(saved["zone_1"]);
   restored.setEnabled(true);
   check(!restored.isEnabled(), "restored overlay replayed");
   nlohmann::json round_trip;
   restored.serializeState(round_trip);
   check(round_trip == saved, "overlay round trip lost progress");
   restored.deserializeState(nlohmann::json::object());
   restored.setEnabled(true);
   check(restored.isEnabled(), "legacy overlay blocked");

   auto repeat = std::make_shared<TmxProperty>();
   repeat->_value_bool = false;
   data._tmx_object->_properties->_map["show_once"] = repeat;
   auto repeatable = InfoOverlay::setup(nullptr, data);
   repeatable->setEnabled(true);
   repeatable->update(sf::seconds(20));
   repeatable->setEnabled(true);
   check(repeatable->isEnabled(), "repeatable overlay blocked");
   nlohmann::json repeat_saved;
   repeatable->serializeState(repeat_saved);
   check(repeat_saved.empty(), "repeatable overlay saved as completed");
}

void test_chest_reward(bool collected)
{
   auto data = make_data("locked_box");
   set_string(data, "spawn_extra", "headtorch");
   auto offset = std::make_shared<TmxProperty>();
   offset->_value_float = 24.0f;
   data._tmx_object->_properties->_map["spawn_offset_x"] = offset;
   TreasureChest chest;
   chest.deserialize(data);

   auto reward = std::make_shared<Extra>();
   reward->_name = "headtorch";
   reward->_spawn_required = true;
   reward->_rect = {{100.0f, 200.0f}, {24.0f, 24.0f}};
   reward->deserializeState({{"active", !collected}});
   std::vector<std::shared_ptr<GameMechanism>> extras{reward};
   chest.restore_reward(extras);
   check(!reward->_spawned, "closed chest exposed its reward");
   chest.deserializeState({{"open", true}});
   chest.restore_reward(extras);
   check(reward->_spawned == !collected, "open chest failed to respect reward pickup state");
   check(reward->isInteractionAvailable() == !collected, "restored reward availability is wrong");
   const auto position = reward->_rect.position;
   check(position.x == (collected ? 100.0f : 124.0f), "restored reward lost its spawn offset");
   chest.restore_reward(extras);
   check(reward->_rect.position == position, "restoring twice applied the offset twice");
}

void test_script_properties()
{
   Extra extra;
   extra.deserializeState({{"active", false}});
   check(!std::get<bool>(*extra.getProperty("active")), "collected extra not exposed to scripts");
   Lever lever;
   lever.deserializeState({{"state", 1}, {"handle_available", true}});
   check(std::get<bool>(*lever.getProperty("handle_available")), "inserted handle not exposed to scripts");
}

void run_lua(const char* filename)
{
   auto* state = luaL_newstate();
   luaL_openlibs(state);
   if (luaL_dofile(state, filename) != LUA_OK)
   {
      const std::string error = lua_tostring(state, -1);
      lua_close(state);
      throw std::runtime_error(error);
   }
   lua_close(state);
}
}  // namespace

int main()
{
   try
   {
      run_lua("tests/lua/test_monk_encounter.lua");
      run_lua("tests/lua/test_progression_restore.lua");
      test_overlay();
      test_script_properties();
      test_chest_reward(false);
      test_chest_reward(true);
      std::cout << "PASS: overlays, script properties, uncollected and collected chest rewards\n";
      return 0;
   }
   catch (const std::exception& error)
   {
      std::cerr << error.what() << '\n';
      return 1;
   }
}
