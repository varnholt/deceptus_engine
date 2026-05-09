// levelscriptcallbacks
#include "levelscriptcallbacks.h"

#include <cstdlib>
#include <lua.hpp>
#include <sstream>

#include "SFML/Graphics.hpp"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"
#include "game/audio/musicplayertypes.h"
#include "game/level/levelscript.h"

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
