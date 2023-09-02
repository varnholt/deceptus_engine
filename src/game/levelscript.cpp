#include "levelscript.h"

#include "framework/tools/log.h"
#include "game/luaconstants.h"
#include "game/savestate.h"

namespace
{

std::mutex instance_mutex;
LevelScript* instance = nullptr;

LevelScript* getInstance()
{
   std::lock_guard<std::mutex> lock(instance_mutex);
   return instance;
}

void setInstance(LevelScript* newInstance)
{
   std::lock_guard<std::mutex> lock(instance_mutex);
   instance = newInstance;
}

void resetInstance()
{
   std::lock_guard<std::mutex> lock(instance_mutex);
   instance = nullptr;
}

/**
 * @brief addCollisionRect add a collision rect that fires when the player intersects
 * @param state lua state
 *    param 1: x position relative to where the object has been placed
 *    param 2: y position relative to where the object has been placed
 *    param 3: collision rect width
 *    param 4: collision rect height
 * @return collision rect id
 */
int32_t addCollisionRect(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   const auto x_px = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto y_px = static_cast<int32_t>(lua_tointeger(state, 2));
   const auto w_px = static_cast<int32_t>(lua_tointeger(state, 3));
   const auto h_px = static_cast<int32_t>(lua_tointeger(state, 4));

   return getInstance()->addCollisionRect({x_px, y_px, w_px, h_px});
}

/**
 * @brief isMechanismEnabled check if a given mechanism is enabled
 * @param state lua state
 *    param 1: mechanism ID
 *    return \c true if mechanism is enabled
 * @return error code
 */
int32_t isMechanismEnabled(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 2)
   {
      return 0;
   }

   const auto id = lua_tostring(state, 1);

   return getInstance()->isMechanismEnabled(id);
}

/**
 * @brief setMechanismEnabled set a mechanism node to enabled/disabled
 * @param state lua state
 *    param 1: enabled flag
 * @return error code
 */
int32_t setMechanismEnabled(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 2)
   {
      return 0;
   }

   const auto id = lua_tostring(state, 1);
   const auto enabled = lua_toboolean(state, 2);

   getInstance()->setMechanismEnabled(id, enabled);
   return 0;
}

/**
 * @brief addSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t addSkill(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto skill = lua_tointeger(state, 1);

   getInstance()->addSkill(skill);
   return 0;
}

/**
 * @brief removeSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t removeSkill(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto skill = lua_tointeger(state, 1);

   getInstance()->removeSkill(skill);
   return 0;
}

[[noreturn]] void error(lua_State* state, const char* /*scope*/ = nullptr)
{
   // the error message is on top of the stack.
   // fetch it, print32_t it and then pop it off the stack.
   std::stringstream os;
   os << lua_tostring(state, -1);

   Log::Error() << os.str();

   lua_pop(state, 1);

   exit(1);
}

}  // namespace

void LevelScript::update(const sf::Time& dt)
{
   // this might be a valid scenario. not every level needs a script to drive its logic.
   if (!_initialized)
   {
      return;
   }

   luaUpdate(dt);
}

LevelScript::LevelScript()
{
   // lua is really c style
   setInstance(this);
}

LevelScript::~LevelScript()
{
   resetInstance();
}

void LevelScript::setup(const std::filesystem::path& path)
{
   _script_name = path.string();

   _lua_state = luaL_newstate();

   // register callbacks
   lua_register(_lua_state, "addCollisionRect", ::addCollisionRect);
   lua_register(_lua_state, "isMechanismEnabled", ::isMechanismEnabled);
   lua_register(_lua_state, "setMechanismEnabled", ::setMechanismEnabled);

   // make standard libraries available in the Lua object
   luaL_openlibs(_lua_state);

   // load program
   auto result = luaL_loadfile(_lua_state, _script_name.c_str());
   if (result == LUA_OK)
   {
      // execute program
      result = lua_pcall(_lua_state, 0, LUA_MULTRET, 0);

      if (result != LUA_OK)
      {
         error(_lua_state);
      }
      else
      {
         luaInitialize();

         // register properties
         for (const auto& prop : _properties)
         {
            luaWriteProperty(prop._name, prop._value);
         }
      }
   }
}

/**
 * @brief LevelScript::luaInitialize called to call the initialize function inside the lua script
 * callback name: initialize
 */
void LevelScript::luaInitialize()
{
   lua_getglobal(_lua_state, FUNCTION_INITIALIZE);
   auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_INITIALIZE);
   }

   _initialized = true;
}

/**
 * @brief LevelScript::luaUpdate update the lua node
 * @param dt delta time, passed to luanode in seconds
 * callback name: update
 */
void LevelScript::luaUpdate(const sf::Time& dt)
{
   lua_getglobal(_lua_state, FUNCTION_UPDATE);
   lua_pushnumber(_lua_state, dt.asSeconds());

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_UPDATE);
   }
}

/**
 * @brief LuaNode::luaWriteProperty write a property of the luanode
 * @param key property key
 * @param value property value
 * callback name: writeProperty
 */
void LevelScript::luaWriteProperty(const std::string& key, const std::string& value)
{
   lua_getglobal(_lua_state, FUNCTION_WRITE_PROPERTY);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, key.c_str());
      lua_pushstring(_lua_state, value.c_str());

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_WRITE_PROPERTY);
      }
   }
}

/**
 * @brief LuaNode::luaPlayerReceivedExtra called when player received an extra
 * @param extra_name name of the extra
 */
void LevelScript::luaPlayerReceivedExtra(const std::string& extra_name)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_RECEIVED_EXTRA);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, extra_name.c_str());

      const auto result = lua_pcall(_lua_state, 1, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_PLAYER_RECEIVED_EXTRA);
      }
   }
}

int32_t LevelScript::addCollisionRect(const sf::IntRect& rect)
{
   _collision_rects.push_back(rect);
   return static_cast<int32_t>(_collision_rects.size());
}

void LevelScript::setMechanismEnabled(const std::string& mechanism_id, bool enabled)
{
   (void)mechanism_id;
   (void)enabled;
}

bool LevelScript::isMechanismEnabled(const std::string& mechanism_id) const
{
   (void)mechanism_id;
   return true;
}

void LevelScript::addSkill(int32_t skill)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills |= skill;
}

void LevelScript::removeSkill(int32_t skill)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills &= ~skill;
}
