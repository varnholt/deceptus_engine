#include "levelscript.h"

#include "framework/tools/log.h"

namespace
{

const std::string FUNCTION_INITIALIZE = "initialize";
const std::string FUNCTION_UPDATE = "update";

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

   return LevelScript::getInstance().addCollisionRect({x_px, y_px, w_px, h_px});
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

}  // namespace

void LevelScript::update(const sf::Time& dt)
{
   luaUpdate(dt);
}

void LevelScript::setup(const std::filesystem::path& path)
{
   _script_name = path.string();

   _lua_state = luaL_newstate();

   // register callbacks
   lua_register(_lua_state, "addCollisionRect", ::addCollisionRect);

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

int32_t LevelScript::addCollisionRect(const sf::IntRect& rect)
{
   _collision_rects.push_back(rect);
   return _collision_rects.size();
}

LevelScript& LevelScript::getInstance()
{
   static LevelScript __level_script;
   return __level_script;
}
