#include "levelscript.h"

#include "framework/tools/log.h"

namespace
{

const std::string FUNCTION_INITIALIZE = "initialize";
const std::string FUNCTION_UPDATE = "update";


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
   if (argc != 1)
   {
      return 0;
   }

   // const auto id = lua_tostring(state, 1);
   // find mechanism with given id

   return 1;
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

   // const auto id = lua_tostring(state, 1);
   // const auto active = static_cast<bool>(lua_toboolean(state, 2));
   // node->setActive(active);

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

}


void LevelScript::update(const sf::Time& dt)
{
   luaUpdate(dt);
}


void LevelScript::setup(const std::filesystem::path& path)
{
   _script_name = path.string();

   _lua_state = luaL_newstate();

   // register callbacks
   // lua_register(_lua_state, "boom", ::boom);

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
         // luaMovedTo();
         luaInitialize();
      }
   }
   else
   {
      error(_lua_state);
   }
}


/**
 * @brief LevelScript::luaInitialize called to call the initialize function inside the lua script
 * callback name: initialize
 */
void LevelScript::luaInitialize()
{
   lua_getglobal(_lua_state, FUNCTION_INITIALIZE.c_str());
   auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_INITIALIZE.c_str());
   }
}


/**
 * @brief LevelScript::luaUpdate update the lua node
 * @param dt delta time, passed to luanode in seconds
 * callback name: update
 */
void LevelScript::luaUpdate(const sf::Time& dt)
{
   lua_getglobal(_lua_state, FUNCTION_UPDATE.c_str());
   lua_pushnumber(_lua_state, dt.asSeconds());

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_UPDATE.c_str());
   }
}



