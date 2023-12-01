#include "levelscript.h"

#include "framework/tools/log.h"
#include "game/bow.h"
#include "game/luaconstants.h"
#include "game/luainterface.h"
#include "game/luanode.h"
#include "game/player/player.h"
#include "game/savestate.h"
#include "game/weaponfactory.h"
#include "game/weaponsystem.h"
#include "mechanisms/sensorrect.h"

#include <regex>

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

   const auto rect_id = getInstance()->addCollisionRect({x_px, y_px, w_px, h_px});
   lua_pushinteger(state, rect_id);
   return 1;
}

/**
 * @brief addSensorRectCallback add a callback when player intersects with a given sensor rect
 * @param state lua state
 *    param 1: identifier of the sensor rect
 * @return error code
 */
int32_t addSensorRectCallback(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto rect_id = lua_tostring(state, 1);
   getInstance()->addSensorRectCallback(rect_id);
   return 0;
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
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const auto search_pattern = lua_tostring(state, 1);

   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   const auto enabled = getInstance()->isMechanismEnabled(search_pattern, group);
   lua_pushboolean(state, enabled);
   return 1;
}

/**
 * @brief setMechanismEnabled set a mechanism node to enabled/disabled
 * @param state lua state
 *    param 1: search pattern
 *    param 2: enabled flag
 * @return error code
 */
int32_t setMechanismEnabled(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 2 || argc > 3)
   {
      return 0;
   }

   const auto search_pattern = lua_tostring(state, 1);
   const auto enabled = lua_toboolean(state, 2);

   std::optional<std::string> group;
   if (argc == 3)
   {
      group = lua_tostring(state, 3);
   }

   getInstance()->setMechanismEnabled(search_pattern, enabled, group);
   return 0;
}

/**
 * @brief toggle toggle a mechanism
 * @param state lua state
 *    param 1: mechanism name
 *    param 2: group name
 * @return error code
 */
int32_t toggle(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 1 || argc > 2)
   {
      return 0;
   }

   const auto search_pattern = lua_tostring(state, 1);

   std::optional<std::string> group;
   if (argc == 2)
   {
      group = lua_tostring(state, 2);
   }

   getInstance()->toggle(search_pattern, group);
   return 0;
}

/**
 * @brief writeLuaNodeProperty write a property of another lua node
 * @param state lua state
 *    param 1: property key
 *    param 2: property value
 *    param 3: mechanism name
 *    param 4: group name
 * @return error code
 */
int32_t writeLuaNodeProperty(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   const auto key = lua_tostring(state, 1);
   const auto value = lua_tostring(state, 2);
   const auto search_pattern = lua_tostring(state, 3);

   getInstance()->writeLuaNodeProperty(key, value, search_pattern);
   return 0;
}

/**
 * @brief addPlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t addPlayerSkill(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto skill = static_cast<int32_t>(lua_tointeger(state, 1));

   getInstance()->addPlayerSkill(skill);
   return 0;
}

/**
 * @brief removePlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t removePlayerSkill(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto skill = static_cast<int32_t>(lua_tointeger(state, 1));

   getInstance()->removePlayerSkill(skill);
   return 0;
}

/**
 * @brief giveWeaponBow give bow to player
 * @return error code
 */
int32_t giveWeaponBow(lua_State* /*state*/)
{
   getInstance()->giveWeaponBow();
   return 0;
}

/**
 * @brief giveWeaponGun give gun to player
 * @return error code
 */
int32_t giveWeaponGun(lua_State* /*state*/)
{
   getInstance()->giveWeaponGun();
   return 0;
}

/**
 * @brief giveWeaponSword give sword to player
 * @return error code
 */
int32_t giveWeaponSword(lua_State* /*state*/)
{
   getInstance()->giveWeaponSword();
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

   const auto player_rect = Player::getCurrent()->getPixelRectInt();
   auto id = 0;
   for (const auto& rect : _collision_rects)
   {
      if (player_rect.intersects(rect))
      {
         luaPlayerCollidesWithRect(id);
      };

      id++;
   }
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
   lua_register(_lua_state, "addSensorRectCallback", ::addSensorRectCallback);
   lua_register(_lua_state, "isMechanismEnabled", ::isMechanismEnabled);
   lua_register(_lua_state, "setMechanismEnabled", ::setMechanismEnabled);
   lua_register(_lua_state, "addPlayerSkill", ::addPlayerSkill);
   lua_register(_lua_state, "removePlayerSkill", ::removePlayerSkill);
   lua_register(_lua_state, "giveWeaponBow", ::giveWeaponBow);
   lua_register(_lua_state, "giveWeaponGun", ::giveWeaponGun);
   lua_register(_lua_state, "giveWeaponSword", ::giveWeaponSword);
   lua_register(_lua_state, "toggle", ::toggle);
   lua_register(_lua_state, "writeLuaNodeProperty", ::writeLuaNodeProperty);

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

void LevelScript::setSearchMechanismCallback(const SearchMechanismCallback& callback)
{
   _search_mechanism_callback = callback;
}

int32_t LevelScript::addCollisionRect(const sf::IntRect& rect)
{
   _collision_rects.push_back(rect);
   return static_cast<int32_t>(_collision_rects.size());
}

void LevelScript::setMechanismEnabled(const std::string& search_pattern, bool enabled, const std::optional<std::string>& group)
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return;
   }

   auto mechanisms = _search_mechanism_callback(search_pattern, group);

   for (auto& mechanism : mechanisms)
   {
      mechanism->setEnabled(enabled);
   }
}

bool LevelScript::isMechanismEnabled(const std::string& search_pattern, const std::optional<std::string>& group) const
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return false;
   }

   auto mechanisms = _search_mechanism_callback(search_pattern, group);
   if (mechanisms.empty())
   {
      return false;
   }
   return mechanisms.front()->isEnabled();
}

void LevelScript::toggle(const std::string& search_pattern, const std::optional<std::string>& group)
{
   auto mechanisms = _search_mechanism_callback(search_pattern, group);
   for (auto& mechanism : mechanisms)
   {
      mechanism->toggle();
   }
}

void LevelScript::addPlayerSkill(int32_t skill)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills |= skill;
}

void LevelScript::removePlayerSkill(int32_t skill)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills &= ~skill;
}

namespace
{
void giveWeaponToPlayer(const std::shared_ptr<Weapon>& weapon)
{
   Player::getCurrent()->getWeaponSystem()->_weapons.push_back(weapon);
   Player::getCurrent()->getWeaponSystem()->_selected = weapon;
}
}  // namespace

void LevelScript::giveWeaponBow()
{
   auto bow = WeaponFactory::create(WeaponType::Bow);
   bow->initialize();
   std::dynamic_pointer_cast<Bow>(bow)->setLauncherBody(Player::getCurrent()->getBody());
   giveWeaponToPlayer(bow);
}

void LevelScript::giveWeaponGun()
{
   auto gun = WeaponFactory::create(WeaponType::Gun);
   gun->initialize();
   giveWeaponToPlayer(gun);
}

void LevelScript::giveWeaponSword()
{
   auto sword = WeaponFactory::create(WeaponType::Sword);
   sword->initialize();
   giveWeaponToPlayer(sword);
}

void LevelScript::writeLuaNodeProperty(const std::string& key, const std::string& value, const std::string& search_pattern)
{
   std::vector<std::shared_ptr<LuaNode>> results;

   const auto& object_list = LuaInterface::instance().getObjectList();
   std::regex pattern(search_pattern);
   for (auto& node : object_list)
   {
      auto lua_node = std::dynamic_pointer_cast<LuaNode>(node);
      if (std::regex_match(lua_node->getObjectId(), pattern))
      {
         results.push_back(lua_node);
      }
   }

   for (auto& lua_node : results)
   {
      lua_node->luaWriteProperty(key, value);
   }
}

void LevelScript::addSensorRectCallback(const std::string& search_pattern)
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return;
   }

   auto mechanisms = _search_mechanism_callback(search_pattern, "sensor_rects");
   for (auto& mechanism : mechanisms)
   {
      auto sensor_rect = std::dynamic_pointer_cast<SensorRect>(mechanism);
      if (sensor_rect)
      {
         sensor_rect->addSensorCallback([this](const std::string& rect_id) { luaPlayerCollidesWithSensorRect(rect_id); });
      }
   }
}

void LevelScript::luaPlayerCollidesWithRect(int32_t rect_id)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_RECT);
   lua_pushnumber(_lua_state, rect_id);

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_RECT);
   }
}

void LevelScript::luaPlayerCollidesWithSensorRect(const std::string& sensor_rect_id)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_SENSOR_RECT);
   lua_pushstring(_lua_state, sensor_rect_id.c_str());

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_SENSOR_RECT);
   }
}
