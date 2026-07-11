#include "levelscript.h"
#include "levelscriptcallbacks.h"

#include "framework/tools/callbackmap.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/audio/audio.h"
#include "game/audio/musicfilenames.h"
#include "game/audio/musicplayer.h"
#include "game/camera/camerasystem.h"
#include "game/camera/camerazoom.h"
#include "game/constants.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/lightsystem.h"
#include "game/effects/screentransition.h"
#include "game/effects/screentransitioneffect.h"
#include "game/io/eventserializer.h"
#include "game/level/levelregistry.h"
#include "game/level/luaconstants.h"
#include "game/level/luainterface.h"
#include "game/level/luanode.h"
#include "game/mechanisms/dialogue.h"
#include "game/mechanisms/extra.h"
#include "game/mechanisms/ringshaderlayer.h"
#include "game/mechanisms/sensorrect.h"
#include "game/player/playercontrols.h"
#include "game/player/playerregistry.h"
#include "game/player/weaponsystem.h"
#include "game/state/displaymode.h"
#include "game/state/savestate.h"
#include "game/weapons/bow.h"
#include "game/weapons/weaponfactory.h"
#include "json/json.hpp"

#include <cmath>
#include <fstream>
#include <mutex>
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

}  // namespace

LevelScript* LevelScript::getCurrent()
{
   return getInstance();
}

void LevelScript::update(const sf::Time& delta_time)
{
   // this might be a valid scenario. not every level needs a script to drive its logic.
   if (!_initialized)
   {
      return;
   }

   luaUpdate(delta_time);
   updateCutsceneSprites(delta_time);

   const auto player_rect = PlayerRegistry::getFirst()->getPixelRectInt();
   auto id = 0;
   for (const auto& rect : _collision_rects)
   {
      if (sfcompat::findIntersection(player_rect, rect))
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

   // add 'item added' callback
   auto& inventory = SaveState::getPlayerInfo()._inventory;
   _inventory_added_callback = [this](const std::string& item) { luaPlayerReceivedItem(item); };
   _inventory_used_callback = [this](const std::string& item) { return luaPlayerUsedItem(item); };
   inventory._added_callbacks.push_back(_inventory_added_callback);
   inventory._used_callbacks.push_back(_inventory_used_callback);

   GameMechanismObserver::clear();

   _enabled_observer_reference = GameMechanismObserver::addListener<GameMechanismObserver::EnabledCallback>(
      [this](const std::string& object_id, const std::string& group_id, bool enabled) { luaMechanismEnabled(object_id, group_id, enabled); }
   );

   _event_observer_reference = GameMechanismObserver::addListener<GameMechanismObserver::EventCallback>(
      [this](const std::string& object_id, const std::string& group_id, const std::string& event_name, const LuaVariant& value)
      { luaMechanismEvent(object_id, group_id, event_name, value); }
   );
}

LevelScript::~LevelScript()
{
   resetInstance();

   // remove 'item added' callback
   auto& inventory = SaveState::getPlayerInfo()._inventory;
   inventory.removeAddedCallback(_inventory_added_callback);
   inventory.removeUsedCallback(_inventory_used_callback);

   stopScript();
}

void LevelScript::stopScript()
{
   if (_lua_state)
   {
      lua_close(_lua_state);
      _lua_state = nullptr;
   }
}

void LevelScript::setup(const std::filesystem::path& path)
{
   _script_name = path.string();

   _lua_state = luaL_newstate();

   // register callbacks
   lua_register(_lua_state, "addAchievement", LevelScriptCallbacks::addAchievement);
   lua_register(_lua_state, "hasAchievement", LevelScriptCallbacks::hasAchievement);
   lua_register(_lua_state, "addTreasure", LevelScriptCallbacks::addTreasure);
   lua_register(_lua_state, "hasTreasure", LevelScriptCallbacks::hasTreasure);
   lua_register(_lua_state, "addCollisionRect", LevelScriptCallbacks::addCollisionRect);
   lua_register(_lua_state, "addPlayerSkill", LevelScriptCallbacks::addPlayerSkill);
   lua_register(_lua_state, "addPlayerHealth", LevelScriptCallbacks::addPlayerHealth);
   lua_register(_lua_state, "addPlayerHealthMax", LevelScriptCallbacks::addPlayerHealthMax);
   lua_register(_lua_state, "addSensorRectCallback", LevelScriptCallbacks::addSensorRectCallback);
   lua_register(_lua_state, "giveWeaponBow", LevelScriptCallbacks::giveWeaponBow);
   lua_register(_lua_state, "giveWeaponGun", LevelScriptCallbacks::giveWeaponGun);
   lua_register(_lua_state, "giveWeaponSword", LevelScriptCallbacks::giveWeaponSword);
   lua_register(_lua_state, "inventoryAdd", LevelScriptCallbacks::inventoryAdd);
   lua_register(_lua_state, "inventoryRemove", LevelScriptCallbacks::inventoryRemove);
   lua_register(_lua_state, "inventoryHas", LevelScriptCallbacks::inventoryHas);
   lua_register(_lua_state, "isMechanismEnabled", LevelScriptCallbacks::isMechanismEnabled);
   lua_register(_lua_state, "isMechanismVisible", LevelScriptCallbacks::isMechanismVisible);
   lua_register(_lua_state, "isPlayerIntersectingSensorRect", LevelScriptCallbacks::isPlayerIntersectingSensorRect);
   lua_register(_lua_state, "lockPlayerControls", LevelScriptCallbacks::lockPlayerControls);
   lua_register(_lua_state, "setCutsceneActive", LevelScriptCallbacks::setCutsceneActive);
   lua_register(_lua_state, "fadeOut", LevelScriptCallbacks::fadeOut);
   lua_register(_lua_state, "fadeIn", LevelScriptCallbacks::fadeIn);
   lua_register(_lua_state, "log", LevelScriptCallbacks::debug);
   lua_register(_lua_state, "playMusic", LevelScriptCallbacks::playMusic);
   lua_register(_lua_state, "removePlayerSkill", LevelScriptCallbacks::removePlayerSkill);
   lua_register(_lua_state, "setLuaNodeActive", LevelScriptCallbacks::setLuaNodeActive);
   lua_register(_lua_state, "setLuaNodeVisible", LevelScriptCallbacks::setLuaNodeVisible);
   lua_register(_lua_state, "setMechanismEnabled", LevelScriptCallbacks::setMechanismEnabled);
   lua_register(_lua_state, "setMechanismVisible", LevelScriptCallbacks::setMechanismVisible);
   lua_register(_lua_state, "flashMechanism", LevelScriptCallbacks::flashMechanism);
   lua_register(_lua_state, "setAmbient", LevelScriptCallbacks::setAmbient);
   lua_register(_lua_state, "setZoomFactor", LevelScriptCallbacks::setZoomFactor);
   lua_register(_lua_state, "showDialogue", LevelScriptCallbacks::showDialogue);
   lua_register(_lua_state, "toggle", LevelScriptCallbacks::toggle);
   lua_register(_lua_state, "tr", LevelScriptCallbacks::translate);
   lua_register(_lua_state, "writeLuaNodeProperty", LevelScriptCallbacks::writeLuaNodeProperty);
   lua_register(_lua_state, "playEventRecording", LevelScriptCallbacks::playEventRecording);
   lua_register(_lua_state, "setCameraPosition", LevelScriptCallbacks::setCameraPosition);
   lua_register(_lua_state, "unlockCamera", LevelScriptCallbacks::unlockCamera);
   lua_register(_lua_state, "setPlayerVisible", LevelScriptCallbacks::setPlayerVisible);
   lua_register(_lua_state, "setInfoLayerVisible", LevelScriptCallbacks::setInfoLayerVisible);
   lua_register(_lua_state, "nextLevel", LevelScriptCallbacks::nextLevel);
   lua_register(_lua_state, "playSound", LevelScriptCallbacks::playSound);
   lua_register(_lua_state, "createSprite", LevelScriptCallbacks::createSprite);
   lua_register(_lua_state, "destroySprite", LevelScriptCallbacks::destroySprite);
   lua_register(_lua_state, "setSpriteAnimation", LevelScriptCallbacks::setSpriteAnimation);
   lua_register(_lua_state, "setSpriteVisible", LevelScriptCallbacks::setSpriteVisible);
   lua_register(_lua_state, "moveSpriteAtSpeed", LevelScriptCallbacks::moveSpriteAtSpeed);
   lua_register(_lua_state, "loadCutscene", LevelScriptCallbacks::loadCutscene);
   lua_register(_lua_state, "getCameraCenter", LevelScriptCallbacks::getCameraCenter);
   lua_register(_lua_state, "getMechanismRect", LevelScriptCallbacks::getMechanismRect);

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
         LevelScriptCallbacks::error(_lua_state);
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
   else
   {
      const auto* error_message = lua_tostring(_lua_state, -1);
      Log::Error() << "Failed loading " << _script_name << ": " << (error_message ? error_message : "unknown error");
      lua_pop(_lua_state, 1);
   }
}

void LevelScript::playEventRecording(const std::string& filename)
{
   // load and play a specific event recording file
   std::string filepath = filename;
   if (filepath.find(".dat") == std::string::npos)
   {
      filepath += ".dat";
   }

   const auto& serializer = EventSerializer::getInstance("player");
   if (serializer)
   {
      serializer->deserialize(filepath);
      serializer->play();
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
      LevelScriptCallbacks::error(_lua_state, FUNCTION_INITIALIZE);
   }

   _initialized = true;
}

/**
 * @brief LevelScript::luaUpdate update the lua node
 * @param delta_time delta time, passed to luanode in seconds
 * callback name: update
 */
void LevelScript::luaUpdate(const sf::Time& delta_time)
{
   lua_getglobal(_lua_state, FUNCTION_UPDATE);
   lua_pushnumber(_lua_state, delta_time.asSeconds());

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      LevelScriptCallbacks::error(_lua_state, FUNCTION_UPDATE);
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
         LevelScriptCallbacks::error(_lua_state, FUNCTION_WRITE_PROPERTY);
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
         LevelScriptCallbacks::error(_lua_state, FUNCTION_PLAYER_RECEIVED_EXTRA);
      }
   }
}

///
/// \brief LevelScript::luaPlayerReceivedItem
/// \param item item that was received
///
void LevelScript::luaPlayerReceivedItem(const std::string& item)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_RECEIVED_ITEM);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, item.c_str());

      const auto result = lua_pcall(_lua_state, 1, 0, 0);

      if (result != LUA_OK)
      {
         LevelScriptCallbacks::error(_lua_state, FUNCTION_PLAYER_RECEIVED_ITEM);
      }
   }
}

///
/// \brief LevelScript::luaPlayerUsedItem
/// \param item item that was used
/// \return bool result from Lua function (false if function not defined or error)
///
bool LevelScript::luaPlayerUsedItem(const std::string& item)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_USED_ITEM);
   if (!lua_isfunction(_lua_state, -1))
   {
      lua_pop(_lua_state, 1);
      return false;
   }

   lua_pushstring(_lua_state, item.c_str());

   const auto result = lua_pcall(_lua_state, 1, 1, 0);
   if (result != LUA_OK)
   {
      LevelScriptCallbacks::error(_lua_state, FUNCTION_PLAYER_USED_ITEM);
      lua_pop(_lua_state, 1);
      return false;
   }

   const bool return_value = lua_toboolean(_lua_state, -1);
   lua_pop(_lua_state, 1);

   return return_value;
}

///
/// \brief LevelScript::luaMechanismEnabled
/// \param object_id object identifier
/// \param group object group
/// \param enabled mechanism enabled flag
///
void LevelScript::luaMechanismEnabled(const std::string& object_id, const std::string& group, bool enabled)
{
   if (_lua_state == nullptr)
   {
      return;
   }

   lua_getglobal(_lua_state, FUNCTION_MECHANISM_ENABLED);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, object_id.c_str());
      lua_pushstring(_lua_state, group.c_str());
      lua_pushboolean(_lua_state, enabled);

      const auto result = lua_pcall(_lua_state, 3, 0, 0);

      if (result != LUA_OK)
      {
         LevelScriptCallbacks::error(_lua_state, FUNCTION_MECHANISM_ENABLED);
      }
   }
}

///
/// \brief LevelScript::luaMechanismEvent
/// \param object_id object identifier
/// \param group object group
/// \param event_name event name
/// \param value value that's part of the event
///
void LevelScript::luaMechanismEvent(
   const std::string& object_id,
   const std::string& group,
   const std::string& event_name,
   const LuaVariant& value
)
{
   if (_lua_state == nullptr)
   {
      return;
   }

   lua_getglobal(_lua_state, FUNCTION_MECHANISM_EVENT);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, object_id.c_str());
      lua_pushstring(_lua_state, group.c_str());
      lua_pushstring(_lua_state, event_name.c_str());

      std::visit(
         [this](auto&& arg)
         {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>)
            {
               lua_pushstring(_lua_state, arg.c_str());
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
               lua_pushinteger(_lua_state, static_cast<lua_Integer>(arg));
            }
            else if constexpr (std::is_same_v<T, double>)
            {
               lua_pushnumber(_lua_state, static_cast<lua_Number>(arg));
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
               lua_pushboolean(_lua_state, arg);
            }
         },
         value
      );

      const auto result = lua_pcall(_lua_state, 4, 0, 0);

      if (result != LUA_OK)
      {
         LevelScriptCallbacks::error(_lua_state, FUNCTION_WRITE_PROPERTY);
      }
   }
}

void LevelScript::setSearchMechanismCallback(const SearchMechanismCallback& callback)
{
   _search_mechanism_callback = callback;
}

void LevelScript::createExtraCallbacks(const std::vector<std::shared_ptr<GameMechanism>>& extras)
{
   // handshake between extra mechanism and level script
   for (const auto& extra_mechanism : extras)
   {
      auto extra = std::dynamic_pointer_cast<Extra>(extra_mechanism);
      extra->_callbacks.emplace_back([this](const std::string& extra) { luaPlayerReceivedExtra(extra); });
   }
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

void LevelScript::flashMechanism(const std::string& search_pattern, float red, float green, float blue, float duration_s)
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return;
   }

   const auto mechanisms = _search_mechanism_callback(search_pattern, std::nullopt);
   Log::Info() << "flashMechanism: pattern '" << search_pattern << "' matched " << mechanisms.size() << " mechanism(s)";
   for (auto& mechanism : mechanisms)
   {
      auto* ring = dynamic_cast<RingShaderLayer*>(mechanism.get());
      Log::Info() << "flashMechanism: dynamic_cast " << (ring ? "succeeded" : "FAILED — not a RingShaderLayer");
      if (ring)
      {
         ring->flash(red, green, blue, duration_s);
      }
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

void LevelScript::setMechanismVisible(const std::string& search_pattern, bool visible, const std::optional<std::string>& group)
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return;
   }

   auto mechanisms = _search_mechanism_callback(search_pattern, group);
   for (auto& mechanism : mechanisms)
   {
      mechanism->setVisible(visible);
   }
}

bool LevelScript::isMechanismVisible(const std::string& search_pattern, const std::optional<std::string>& group) const
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
   return mechanisms.front()->isVisible();
}

bool LevelScript::isPlayerIntersectingSensorRect(const std::string& mechanism_id) const
{
   auto mechanisms = _search_mechanism_callback(mechanism_id, "sensor_rects");

   auto mechanism_it = std::ranges::find_if(
      mechanisms,
      [&mechanism_id](const auto& mechanism)
      {
         auto* game_node = dynamic_cast<GameNode*>(mechanism.get());
         return (game_node && game_node->getObjectId() == mechanism_id);
      }
   );

   return mechanism_it != mechanisms.end();
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

void LevelScript::addPlayerHealth(int32_t health_points_to_add)
{
   SaveState::getPlayerInfo()._extra_table._health.addHealth(health_points_to_add);
}

void LevelScript::addPlayerHealthMax(int32_t health_points_to_add)
{
   SaveState::getPlayerInfo()._extra_table._health._health_max += health_points_to_add;
}

void LevelScript::setZoomFactor(float zoom_factor)
{
   CameraZoom::getInstance().setZoomFactor(zoom_factor);
}

void LevelScript::setAmbient(sf::Color color)
{
   LevelRegistry::getCurrent()->getLightSystem()->setAmbient(color);
}

void LevelScript::addAchievement(const std::string& identifier)
{
   SaveState::getPlayerInfo()._achievements.add(identifier);
}

bool LevelScript::hasAchievement(const std::string& identifier)
{
   return SaveState::getPlayerInfo()._achievements.has(identifier);
}

void LevelScript::addTreasure(const std::string& identifier)
{
   SaveState::getPlayerInfo()._treasures.add(identifier);
}

bool LevelScript::hasTreasure(const std::string& identifier)
{
   return SaveState::getPlayerInfo()._treasures.has(identifier);
}

void LevelScript::inventoryAdd(const std::string& item)
{
   auto& inventory = SaveState::getCurrent().getPlayerInfo()._inventory;
   inventory.add(item);
}

void LevelScript::inventoryRemove(const std::string& item)
{
   auto& inventory = SaveState::getCurrent().getPlayerInfo()._inventory;
   inventory.remove(item);
}

bool LevelScript::inventoryHas(const std::string& item)
{
   auto& inventory = SaveState::getCurrent().getPlayerInfo()._inventory;
   return inventory.has(item);
}

void LevelScript::playMusic(
   const std::string& filename,
   MusicPlayerTypes::TransitionType transition_type,
   std::chrono::milliseconds transition_duration,
   MusicPlayerTypes::PostPlaybackAction post_action
)
{
   MusicFilenames::setLevelMusic(filename);
   MusicPlayer::getInstance().queueTrack({filename, transition_type, transition_duration, post_action});
}

namespace
{
void giveWeaponToPlayer(const std::shared_ptr<Weapon>& weapon)
{
   auto& weapon_system = SaveState::getPlayerInfo()._weapons;
   weapon_system._weapons.push_back(weapon);
   weapon_system._selected = weapon;
}
}  // namespace

void LevelScript::giveWeaponBow()
{
   auto bow = WeaponFactory::create(WeaponType::Bow);
   std::dynamic_pointer_cast<Bow>(bow)->setLauncherBody(PlayerRegistry::getFirst()->getBody());
   giveWeaponToPlayer(bow);
}

void LevelScript::giveWeaponGun()
{
   auto gun = WeaponFactory::create(WeaponType::Gun);
   giveWeaponToPlayer(gun);
}

void LevelScript::giveWeaponSword()
{
   auto sword = WeaponFactory::create(WeaponType::Sword);
   giveWeaponToPlayer(sword);
}

std::vector<std::shared_ptr<LuaNode>> LevelScript::findLuaNodes(const std::string& search_pattern)
{
   std::vector<std::shared_ptr<LuaNode>> results;

   const auto& object_list = LuaInterface::instance().getObjectList();
   std::regex pattern(search_pattern);
   for (const auto& node : object_list)
   {
      auto lua_node = std::dynamic_pointer_cast<LuaNode>(node);
      if (std::regex_match(lua_node->_name, pattern))
      {
         results.push_back(lua_node);
      }
   }

   return results;
}

void LevelScript::writeLuaNodeProperty(const std::string& search_pattern, const std::string& key, const std::string& value)
{
   const auto results = findLuaNodes(search_pattern);

   if (results.empty())
   {
      Log::Error() << "search pattern " << search_pattern << " did not give any results";
      return;
   }

   for (const auto& lua_node : results)
   {
      lua_node->luaWriteProperty(key, value);
   }
}

void LevelScript::setLuaNodeVisible(const std::string& search_pattern, bool visible)
{
   const auto results = findLuaNodes(search_pattern);

   if (results.empty())
   {
      Log::Error() << "search pattern " << search_pattern << " did not give any results";
      return;
   }

   for (const auto& lua_node : results)
   {
      lua_node->_visible = visible;
   }
}

void LevelScript::setLuaNodeActive(const std::string& search_pattern, bool active)
{
   const auto results = findLuaNodes(search_pattern);

   if (results.empty())
   {
      Log::Error() << "search pattern " << search_pattern << " did not give any results";
      return;
   }

   for (const auto& lua_node : results)
   {
      lua_node->setActive(active);
   }
}

void LevelScript::showDialogue(const std::string& search_pattern)
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return;
   }

   auto mechanisms = _search_mechanism_callback(search_pattern, "dialogues");

   if (mechanisms.empty())
   {
      Log::Error() << "search pattern " << search_pattern << " did not give any results";
      return;
   }

   auto dialogue = std::dynamic_pointer_cast<Dialogue>(mechanisms.front());
   dialogue->setActive(true);
   dialogue->showNext();
}

void LevelScript::showDialogue(std::vector<Dialogue::DialogueItem> items)
{
   _scripted_dialogue = std::make_shared<Dialogue>();
   _scripted_dialogue->setItems(std::move(items));
   _scripted_dialogue->setActive(true);
   _scripted_dialogue->showNext();
}

void LevelScript::lockPlayerControls(const std::chrono::milliseconds& duration)
{
   PlayerRegistry::getFirst()->getControls()->lockAll(PlayerControls::LockedState::Released, duration);
}

void LevelScript::fadeOut(float speed)
{
   auto transition = std::make_unique<ScreenTransition>();

   auto fade_out = std::make_shared<FadeTransitionEffect>();
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = speed;

   _pending_fade_in = std::make_shared<FadeTransitionEffect>();
   _pending_fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   _pending_fade_in->_value = 1.0f;
   _pending_fade_in->_speed = 1.0f;

   transition->_effect_1 = fade_out;
   transition->_effect_2 = _pending_fade_in;
   transition->_autostart_effect_2 = false;
   transition->_callbacks_effect_1_ended.emplace_back([this]() { luaMechanismEvent("fade", "", "out_done", true); });
   transition->_callbacks_effect_2_ended.emplace_back([]() { ScreenTransitionHandler::getInstance().pop(); });

   transition->startEffect1();
   ScreenTransitionHandler::getInstance().push(std::move(transition));
}

void LevelScript::fadeIn(float speed)
{
   if (_pending_fade_in)
   {
      _pending_fade_in->_speed = speed;
      ScreenTransitionHandler::getInstance().startEffect2();
   }
   else
   {
      auto transition = std::make_unique<ScreenTransition>();
      auto null_effect = std::make_shared<ScreenTransitionEffect>();
      auto fade_in = std::make_shared<FadeTransitionEffect>();
      fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
      fade_in->_value = 1.0f;
      fade_in->_speed = speed;

      transition->_effect_1 = null_effect;
      transition->_effect_2 = fade_in;
      transition->_callbacks_effect_2_ended.emplace_back(
         [this]()
         {
            ScreenTransitionHandler::getInstance().pop();
            luaMechanismEvent("fade", "", "in_done", true);
         }
      );

      transition->startEffect1();
      ScreenTransitionHandler::getInstance().push(std::move(transition));
      ScreenTransitionHandler::getInstance().startEffect2();
   }
}

void LevelScript::setCameraPosition(float x_px, float y_px)
{
   CameraSystem::getInstance().snapTo(x_px, y_px);
}

void LevelScript::unlockCamera()
{
   CameraSystem::getInstance().unlockCamera();
}

sf::Vector2f LevelScript::getCameraCenter() const
{
   return CameraSystem::getInstance().getCenterPx();
}

std::optional<sf::FloatRect> LevelScript::getMechanismRect(const std::string& search_pattern, const std::optional<std::string>& group) const
{
   if (!_search_mechanism_callback)
   {
      Log::Error() << "search mechanism callback not initialized yet";
      return std::nullopt;
   }

   const auto mechanisms = _search_mechanism_callback(search_pattern, group);
   if (mechanisms.empty())
   {
      return std::nullopt;
   }
   return mechanisms.front()->getBoundingBoxPx();
}

void LevelScript::setPlayerVisible(bool visible)
{
   auto player = PlayerRegistry::getFirst();
   if (player)
   {
      player->setVisible(visible);
   }
}

void LevelScript::setHudVisible(bool visible)
{
   if (visible)
   {
      DisplayMode::getInstance().enqueueSet(Display::InfoLayer);
   }
   else
   {
      DisplayMode::getInstance().enqueueUnset(Display::InfoLayer);
   }
}

void LevelScript::nextLevel()
{
   CallbackMap::getInstance().call(static_cast<int32_t>(CallbackType::NextLevel));
}

void LevelScript::playSound(const std::string& sample_name)
{
   Audio::getInstance().playSample(Audio::PlayInfo{sample_name});
}

void LevelScript::createSprite(
   const std::string& name,
   const std::string& animation_file,
   const std::string& animation_id,
   float x_px,
   float y_px,
   bool looped
)
{
   auto pool_iter = _cutscene_pools.find(animation_file);
   if (pool_iter == _cutscene_pools.end())
   {
      auto new_pool = std::make_shared<AnimationPool>(animation_file);
      new_pool->initialize();
      pool_iter = _cutscene_pools.emplace(animation_file, std::move(new_pool)).first;
   }

   CutsceneSprite sprite;
   sprite._name = name;
   sprite._pool = pool_iter->second;
   sprite._animation = pool_iter->second->create(animation_id, x_px, y_px, true, false);
   sprite._animation->_looped = looped;
   sprite._animation->seekToStart();
   sprite._position = {x_px, y_px};
   _cutscene_sprites.push_back(std::move(sprite));
}

void LevelScript::destroySprite(const std::string& name)
{
   std::erase_if(_cutscene_sprites, [&name](const CutsceneSprite& sprite) { return sprite._name == name; });
}

void LevelScript::setSpriteAnimation(const std::string& name, const std::string& animation_id, bool looped)
{
   for (auto& sprite : _cutscene_sprites)
   {
      if (sprite._name != name)
      {
         continue;
      }
      sprite._animation = sprite._pool->create(animation_id, sprite._position.x, sprite._position.y, true, false);
      sprite._animation->_looped = looped;
      sprite._animation->seekToStart();
      return;
   }
}

void LevelScript::setSpriteVisible(const std::string& name, bool visible)
{
   for (auto& sprite : _cutscene_sprites)
   {
      if (sprite._name == name)
      {
         sprite._animation->setVisible(visible);
         return;
      }
   }
}

void LevelScript::moveSpriteAtSpeed(
   const std::string& name,
   float target_x,
   float target_y,
   float speed_px_per_s,
   const std::string& arrive_event
)
{
   for (auto& sprite : _cutscene_sprites)
   {
      if (sprite._name != name)
      {
         continue;
      }
      sprite._target = {target_x, target_y};
      sprite._speed_px_per_s = speed_px_per_s;
      sprite._arrive_event = arrive_event;
      sprite._moving = true;
      return;
   }
}

void LevelScript::draw(sf::RenderTarget& target, const sf::RenderStates& states)
{
   for (const auto& sprite : _cutscene_sprites)
   {
      if (sprite._animation)
      {
         target.draw(*sprite._animation, states);
      }
   }
}

void LevelScript::updateCutsceneSprites(const sf::Time& dt)
{
   for (auto& sprite : _cutscene_sprites)
   {
      if (sprite._animation)
      {
         sprite._animation->update(dt);
      }

      if (!sprite._moving)
      {
         continue;
      }

      const auto direction = sprite._target - sprite._position;
      const auto distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
      const auto step = sprite._speed_px_per_s * dt.asSeconds();

      if (step >= distance)
      {
         sprite._position = sprite._target;
         sprite._moving = false;
         sfcompat::setPosition(*sprite._animation, sprite._position);
         if (!sprite._arrive_event.empty())
         {
            luaRaiseEvent(sprite._arrive_event);
         }
      }
      else
      {
         const auto normalized = direction / distance;
         sprite._position += normalized * step;
         sfcompat::setPosition(*sprite._animation, sprite._position);
      }
   }
}

void LevelScript::luaRaiseEvent(const std::string& event_name)
{
   if (!_initialized)
   {
      return;
   }

   lua_getglobal(_lua_state, "onEvent");
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, event_name.c_str());
      if (lua_pcall(_lua_state, 1, 0, 0) != LUA_OK)
      {
         LevelScriptCallbacks::error(_lua_state, "onEvent");
      }
   }
   else
   {
      lua_pop(_lua_state, 1);
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

   if (mechanisms.empty())
   {
      Log::Error() << "search pattern " << search_pattern << " did not give any results";
      return;
   }

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
      LevelScriptCallbacks::error(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_RECT);
   }
}

void LevelScript::luaPlayerCollidesWithSensorRect(const std::string& sensor_rect_id)
{
   lua_getglobal(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_SENSOR_RECT);
   lua_pushstring(_lua_state, sensor_rect_id.c_str());

   auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      LevelScriptCallbacks::error(_lua_state, FUNCTION_PLAYER_COLLIDES_WITH_SENSOR_RECT);
   }
}
