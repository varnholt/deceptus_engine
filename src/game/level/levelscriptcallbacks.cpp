// levelscriptcallbacks
#include "levelscriptcallbacks.h"

#include <cstdlib>
#include <fstream>
#include <lua.hpp>
#include <sstream>

#include "SFML/Graphics.hpp"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "game/audio/musicplayertypes.h"
#include "game/level/levelscript.h"
#include "game/state/displaymode.h"
#include "json/json.hpp"

namespace LevelScriptCallbacks
{

int32_t addCollisionRect(lua_State* state)
{
   if (lua_gettop(state) != 4)
   {
      return 0;
   }

   const auto x_px = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto y_px = static_cast<int32_t>(lua_tointeger(state, 2));
   const auto w_px = static_cast<int32_t>(lua_tointeger(state, 3));
   const auto h_px = static_cast<int32_t>(lua_tointeger(state, 4));

   const auto rect_id = LevelScript::getCurrent()->addCollisionRect({{x_px, y_px}, {w_px, h_px}});
   lua_pushinteger(state, rect_id);
   return 1;
}

int32_t addSensorRectCallback(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   const std::string rect_id = lua_tostring(state, 1);
   LevelScript::getCurrent()->addSensorRectCallback(rect_id);
   return 0;
}

int32_t isPlayerIntersectingSensorRect(lua_State* state)
{
   if (lua_gettop(state) < 1)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   lua_pushboolean(state, LevelScript::getCurrent()->isPlayerIntersectingSensorRect(search_pattern));
   return 1;
}

int32_t isMechanismEnabled(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   lua_pushboolean(state, LevelScript::getCurrent()->isMechanismEnabled(search_pattern, group));
   return 1;
}

int32_t isMechanismVisible(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   lua_pushboolean(state, LevelScript::getCurrent()->isMechanismVisible(search_pattern, group));
   return 1;
}

int32_t setMechanismEnabled(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 2 || argc > 3)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   const auto enabled = lua_toboolean(state, 2);
   std::optional<std::string> group;
   if (argc == 3)
   {
      group = lua_tostring(state, 3);
   }

   LevelScript::getCurrent()->setMechanismEnabled(search_pattern, enabled, group);
   return 0;
}

int32_t setMechanismVisible(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 2 || argc > 3)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   const auto visible = lua_toboolean(state, 2);
   std::optional<std::string> group;
   if (argc == 3)
   {
      group = lua_tostring(state, 3);
   }

   LevelScript::getCurrent()->setMechanismVisible(search_pattern, visible, group);
   return 0;
}

int32_t flashMechanism(lua_State* state)
{
   if (lua_gettop(state) != 5)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   const auto red = static_cast<float>(lua_tonumber(state, 2));
   const auto green = static_cast<float>(lua_tonumber(state, 3));
   const auto blue = static_cast<float>(lua_tonumber(state, 4));
   const auto duration = static_cast<float>(lua_tonumber(state, 5));

   LevelScript::getCurrent()->flashMechanism(search_pattern, red, green, blue, duration);
   return 0;
}

int32_t toggle(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   LevelScript::getCurrent()->toggle(search_pattern, group);
   return 0;
}

int32_t getMechanismRect(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const std::string search_pattern = lua_tostring(state, 1);
   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   const auto rect = LevelScript::getCurrent()->getMechanismRect(search_pattern, group);
   if (!rect.has_value())
   {
      lua_pushnil(state);
      return 1;
   }

   lua_createtable(state, 0, 4);
   lua_pushnumber(state, rect->position.x);
   lua_setfield(state, -2, "x");
   lua_pushnumber(state, rect->position.y);
   lua_setfield(state, -2, "y");
   lua_pushnumber(state, rect->size.x);
   lua_setfield(state, -2, "width");
   lua_pushnumber(state, rect->size.y);
   lua_setfield(state, -2, "height");
   return 1;
}

int32_t getCameraCenter(lua_State* state)
{
   const auto camera_center = LevelScript::getCurrent()->getCameraCenter();
   lua_createtable(state, 0, 2);
   lua_pushnumber(state, camera_center.x);
   lua_setfield(state, -2, "x");
   lua_pushnumber(state, camera_center.y);
   lua_setfield(state, -2, "y");
   return 1;
}

int32_t showDialogue(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->showDialogue(lua_tostring(state, 1));
   return 0;
}

int32_t addPlayerSkill(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->addPlayerSkill(static_cast<int32_t>(lua_tointeger(state, 1)));
   return 0;
}

int32_t removePlayerSkill(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->removePlayerSkill(static_cast<int32_t>(lua_tointeger(state, 1)));
   return 0;
}

int32_t addPlayerHealth(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->addPlayerHealth(static_cast<int32_t>(lua_tointeger(state, 1)));
   return 0;
}

int32_t addPlayerHealthMax(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->addPlayerHealthMax(static_cast<int32_t>(lua_tointeger(state, 1)));
   return 0;
}

int32_t addAchievement(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->addAchievement(lua_tostring(state, 1));
   return 0;
}

int32_t hasAchievement(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   lua_pushboolean(state, LevelScript::getCurrent()->hasAchievement(lua_tostring(state, 1)));
   return 1;
}

int32_t addTreasure(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->addTreasure(lua_tostring(state, 1));
   return 0;
}

int32_t hasTreasure(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   lua_pushboolean(state, LevelScript::getCurrent()->hasTreasure(lua_tostring(state, 1)));
   return 1;
}

int32_t inventoryAdd(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->inventoryAdd(lua_tostring(state, 1));
   return 0;
}

int32_t inventoryRemove(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->inventoryRemove(lua_tostring(state, 1));
   return 0;
}

int32_t inventoryHas(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   lua_pushboolean(state, LevelScript::getCurrent()->inventoryHas(lua_tostring(state, 1)));
   return 1;
}

int32_t giveWeaponBow(lua_State* /*state*/)
{
   LevelScript::getCurrent()->giveWeaponBow();
   return 0;
}

int32_t giveWeaponGun(lua_State* /*state*/)
{
   LevelScript::getCurrent()->giveWeaponGun();
   return 0;
}

int32_t giveWeaponSword(lua_State* /*state*/)
{
   LevelScript::getCurrent()->giveWeaponSword();
   return 0;
}

int32_t writeLuaNodeProperty(lua_State* state)
{
   if (lua_gettop(state) != 3)
   {
      return 0;
   }

   LevelScript::getCurrent()->writeLuaNodeProperty(lua_tostring(state, 1), lua_tostring(state, 2), lua_tostring(state, 3));
   return 0;
}

int32_t setLuaNodeVisible(lua_State* state)
{
   if (lua_gettop(state) != 2)
   {
      return 0;
   }

   LevelScript::getCurrent()->setLuaNodeVisible(lua_tostring(state, 1), lua_toboolean(state, 2));
   return 0;
}

int32_t setLuaNodeActive(lua_State* state)
{
   if (lua_gettop(state) != 2)
   {
      return 0;
   }

   LevelScript::getCurrent()->setLuaNodeActive(lua_tostring(state, 1), lua_toboolean(state, 2));
   return 0;
}

int32_t playMusic(lua_State* state)
{
   if (lua_gettop(state) != 4)
   {
      return 0;
   }

   const auto filename = std::string(lua_tostring(state, 1));
   const auto transition_type = static_cast<MusicPlayerTypes::TransitionType>(lua_tointeger(state, 2));
   const auto transition_duration = std::chrono::milliseconds(lua_tointeger(state, 3));
   const auto post_action = static_cast<MusicPlayerTypes::PostPlaybackAction>(lua_tointeger(state, 4));

   LevelScript::getCurrent()->playMusic(filename, transition_type, transition_duration, post_action);
   return 0;
}

int32_t lockPlayerControls(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->lockPlayerControls(std::chrono::milliseconds{static_cast<int32_t>(lua_tointeger(state, 1))});
   return 0;
}

int32_t setCutsceneActive(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   const auto active = lua_toboolean(state, 1);
   if (active)
   {
      DisplayMode::getInstance().enqueueSet(Display::CutsceneActive);
   }
   else
   {
      DisplayMode::getInstance().enqueueUnset(Display::CutsceneActive);
   }
   return 0;
}

int32_t fadeOut(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->fadeOut(static_cast<float>(lua_tonumber(state, 1)));
   return 0;
}

int32_t fadeIn(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->fadeIn(static_cast<float>(lua_tonumber(state, 1)));
   return 0;
}

int32_t setAmbient(lua_State* state)
{
   if (lua_gettop(state) != 4)
   {
      return 0;
   }

   const auto r = static_cast<uint8_t>(lua_tointeger(state, 1));
   const auto g = static_cast<uint8_t>(lua_tointeger(state, 2));
   const auto b = static_cast<uint8_t>(lua_tointeger(state, 3));
   const auto a = static_cast<uint8_t>(lua_tointeger(state, 4));
   LevelScript::getCurrent()->setAmbient(sf::Color{r, g, b, a});
   return 0;
}

int32_t setZoomFactor(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->setZoomFactor(static_cast<float>(lua_tonumber(state, 1)));
   return 0;
}

int32_t playEventRecording(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   const char* filename = lua_tostring(state, 1);
   if (filename == nullptr)
   {
      return 0;
   }

   LevelScript::getCurrent()->playEventRecording(std::string(filename));
   return 0;
}

int32_t setCameraPosition(lua_State* state)
{
   if (lua_gettop(state) != 2)
   {
      return 0;
   }

   const auto x_px = static_cast<float>(lua_tonumber(state, 1));
   const auto y_px = static_cast<float>(lua_tonumber(state, 2));
   LevelScript::getCurrent()->setCameraPosition(x_px, y_px);
   return 0;
}

int32_t unlockCamera(lua_State* /*state*/)
{
   LevelScript::getCurrent()->unlockCamera();
   return 0;
}

int32_t setPlayerVisible(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->setPlayerVisible(lua_toboolean(state, 1));
   return 0;
}

int32_t setInfoLayerVisible(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->setHudVisible(lua_toboolean(state, 1));
   return 0;
}

int32_t nextLevel(lua_State* /*state*/)
{
   LevelScript::getCurrent()->nextLevel();
   return 0;
}

int32_t playSound(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->playSound(lua_tostring(state, 1));
   return 0;
}

int32_t createSprite(lua_State* state)
{
   if (lua_gettop(state) != 6)
   {
      return 0;
   }

   const std::string name = lua_tostring(state, 1);
   const std::string animation_file = lua_tostring(state, 2);
   const std::string animation_id = lua_tostring(state, 3);
   const auto x_px = static_cast<float>(lua_tonumber(state, 4));
   const auto y_px = static_cast<float>(lua_tonumber(state, 5));
   const bool looped = lua_toboolean(state, 6);
   LevelScript::getCurrent()->createSprite(name, animation_file, animation_id, x_px, y_px, looped);
   return 0;
}

int32_t destroySprite(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   LevelScript::getCurrent()->destroySprite(lua_tostring(state, 1));
   return 0;
}

int32_t setSpriteAnimation(lua_State* state)
{
   if (lua_gettop(state) != 3)
   {
      return 0;
   }

   LevelScript::getCurrent()->setSpriteAnimation(lua_tostring(state, 1), lua_tostring(state, 2), lua_toboolean(state, 3));
   return 0;
}

int32_t setSpriteVisible(lua_State* state)
{
   if (lua_gettop(state) != 2)
   {
      return 0;
   }

   LevelScript::getCurrent()->setSpriteVisible(lua_tostring(state, 1), lua_toboolean(state, 2));
   return 0;
}

int32_t moveSpriteAtSpeed(lua_State* state)
{
   if (lua_gettop(state) != 5)
   {
      return 0;
   }

   const std::string name = lua_tostring(state, 1);
   const auto target_x = static_cast<float>(lua_tonumber(state, 2));
   const auto target_y = static_cast<float>(lua_tonumber(state, 3));
   const auto speed_px_per_s = static_cast<float>(lua_tonumber(state, 4));
   const std::string arrive_event = lua_tostring(state, 5);
   LevelScript::getCurrent()->moveSpriteAtSpeed(name, target_x, target_y, speed_px_per_s, arrive_event);
   return 0;
}

int32_t loadCutscene(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   const std::string path = lua_tostring(state, 1);
   std::ifstream file_stream(path);
   if (!file_stream.is_open())
   {
      Log::Error() << "loadCutscene: cannot open " << path;
      return 0;
   }

   nlohmann::json json_data;
   try
   {
      json_data = nlohmann::json::parse(file_stream);
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "loadCutscene: parse error in " << path << ": " << exception.what();
      return 0;
   }

   lua_newtable(state);
   int32_t array_index = 1;
   for (const auto& entry : json_data)
   {
      lua_newtable(state);
      for (const auto& [key, value] : entry.items())
      {
         lua_pushstring(state, key.c_str());
         if (value.is_string())
         {
            lua_pushstring(state, value.get<std::string>().c_str());
         }
         else if (value.is_number_float())
         {
            lua_pushnumber(state, value.get<double>());
         }
         else if (value.is_number_integer())
         {
            lua_pushinteger(state, value.get<int64_t>());
         }
         else if (value.is_boolean())
         {
            lua_pushboolean(state, value.get<bool>() ? 1 : 0);
         }
         else
         {
            lua_pop(state, 1);
            continue;
         }
         lua_settable(state, -3);
      }
      lua_rawseti(state, -2, array_index);
      array_index++;
   }
   return 1;
}

int32_t debug(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   Log::Info() << lua_tostring(state, 1);
   return 0;
}

int32_t translate(lua_State* state)
{
   if (lua_gettop(state) != 1)
   {
      return 0;
   }

   const auto translated = tr(lua_tostring(state, 1));
   lua_pushstring(state, translated.c_str());
   return 1;
}

[[noreturn]] void error(lua_State* state, const char* /*scope*/)
{
   std::stringstream output_stream;
   output_stream << lua_tostring(state, -1);
   Log::Error() << output_stream.str();
   lua_pop(state, 1);
   exit(1);
}

}  // namespace LevelScriptCallbacks
