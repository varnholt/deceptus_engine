#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <lua.hpp>
#include <string>
#include <variant>

#include "game/audio/musicplayertypes.h"
#include "game/level/scriptproperty.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/gamemechanismobserver.h"

struct LuaNode;

/// \brief runtime bridge between level lua scripts and engine systems.
class LevelScript
{
public:
   LevelScript();
   ~LevelScript();

   /// \brief loads a level lua script, registers c++ callbacks, and runs script initialization.
   /// \param path path to the lua script file.
   void setup(const std::filesystem::path& path);
   /// \brief updates the script and triggers collision callbacks for registered collision rects.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   /// \brief registers a player-collision rectangle used for lua collision events.
   /// \param rect collision rectangle in pixel coordinates.
   /// \return 1-based id of the inserted rectangle.
   int32_t addCollisionRect(const sf::IntRect& rect);
   /// \brief subscribes matching sensor rect mechanisms to forward enter events to lua.
   /// \param sensor_rect_id regex pattern used to find sensor rect mechanisms.
   void addSensorRectCallback(const std::string& sensor_rect_id);
   /// \brief sets the enabled state for all matching mechanisms.
   /// \param search_pattern regex used to select mechanisms.
   /// \param enabled target enabled state.
   /// \param group optional mechanism group filter.
   void setMechanismEnabled(const std::string& search_pattern, bool enabled, const std::optional<std::string>& group = std::nullopt);
   /// \brief checks enabled state of the first mechanism that matches the query.
   /// \param mechanism_id regex used to select mechanisms.
   /// \param group optional mechanism group filter.
   /// \return true when at least one mechanism matches and the first one is enabled.
   bool isMechanismEnabled(const std::string& mechanism_id, const std::optional<std::string>& group) const;
   /// \brief sets visibility for all matching mechanisms.
   /// \param search_pattern regex used to select mechanisms.
   /// \param visible target visibility state.
   /// \param group optional mechanism group filter.
   void setMechanismVisible(const std::string& search_pattern, bool visible, const std::optional<std::string>& group = std::nullopt);
   /// \brief checks visibility of the first mechanism that matches the query.
   /// \param mechanism_id regex used to select mechanisms.
   /// \param group optional mechanism group filter.
   /// \return true when at least one mechanism matches and the first one is visible.
   bool isMechanismVisible(const std::string& mechanism_id, const std::optional<std::string>& group) const;
   /// \brief checks whether a sensor rect mechanism with the given id exists in the sensor group.
   /// \param mechanism_id sensor rect object id.
   /// \return true when a matching sensor rect mechanism is found.
   bool isPlayerIntersectingSensorRect(const std::string& mechanism_id) const;
   /// \brief toggles all matching mechanisms.
   /// \param search_pattern regex used to select mechanisms.
   /// \param group optional mechanism group filter.
   void toggle(const std::string& search_pattern, const std::optional<std::string>& group = std::nullopt);
   /// \brief adds one or more player skill flags to save-state player data.
   /// \param skill bitmask of skills to add.
   void addPlayerSkill(int32_t skill);
   /// \brief removes one or more player skill flags from save-state player data.
   /// \param skill bitmask of skills to remove.
   void removePlayerSkill(int32_t skill);
   /// \brief increases current player health in save-state data.
   /// \param health_points_to_add amount of health to add.
   void addPlayerHealth(int32_t health_points_to_add);
   /// \brief increases maximum player health in save-state data.
   /// \param health_points_to_add amount to add to max health.
   void addPlayerHealthMax(int32_t health_points_to_add);
   /// \brief creates a bow and equips it as the selected player weapon.
   void giveWeaponBow();
   /// \brief creates a gun and equips it as the selected player weapon.
   void giveWeaponGun();
   /// \brief creates a sword and equips it as the selected player weapon.
   void giveWeaponSword();
   /// \brief writes a property into all LuaNode scripts that match the pattern.
   /// \param search_pattern regex used to find LuaNode instances by name.
   /// \param key property key forwarded to lua.
   /// \param value property value forwarded to lua.
   void writeLuaNodeProperty(const std::string& search_pattern, const std::string& key, const std::string& value);
   /// \brief sets visibility on all LuaNode instances that match the pattern.
   /// \param search_pattern regex used to find LuaNode instances by name.
   /// \param visible target visibility state.
   void setLuaNodeVisible(const std::string& search_pattern, bool visible);
   /// \brief enables or disables physics activity on matching LuaNode instances.
   /// \param search_pattern regex used to find LuaNode instances by name.
   /// \param active true to enable the body, false to disable it.
   void setLuaNodeActive(const std::string& search_pattern, bool active);
   /// \brief activates the first matching dialogue mechanism and advances to its next page.
   /// \param search_pattern regex used to find dialogue mechanisms.
   void showDialogue(const std::string& search_pattern);
   /// \brief locks player controls for a fixed duration.
   /// \param duration lock duration.
   void lockPlayerControls(const std::chrono::milliseconds& duration);
   /// \brief sets camera zoom factor.
   /// \param zoom_factor absolute zoom multiplier.
   void setZoomFactor(float zoom_factor);
   /// \brief adds an item to the player inventory.
   /// \param item inventory item id.
   void inventoryAdd(const std::string& item);
   /// \brief removes an item from the player inventory.
   /// \param item inventory item id.
   void inventoryRemove(const std::string& item);
   /// \brief checks whether an item is currently in the player inventory.
   /// \param item inventory item id.
   /// \return true when the item exists in inventory.
   bool inventoryHas(const std::string& item);
   /// \brief queues level music playback with transition settings.
   /// \param filename music filename or id.
   /// \param transition_type transition type used when starting playback.
   /// \param transition_duration duration of the transition.
   /// \param post_action behavior after track playback.
   void playMusic(
      const std::string& filename,
      MusicPlayerTypes::TransitionType transition_type,
      std::chrono::milliseconds transition_duration,
      MusicPlayerTypes::PostPlaybackAction post_action
   );
   /// \brief loads and starts playback of a recorded player event stream.
   /// \param filename recording file name with or without .dat extension.
   void playEventRecording(const std::string& filename);

   // functions on the lua end
   /// \brief calls the script initialize function and marks this script as initialized.
   void luaInitialize();
   /// \brief calls the script update function with delta time in seconds.
   /// \param dt elapsed frame time.
   void luaUpdate(const sf::Time& dt);
   /// \brief forwards a key-value property pair to the script writeProperty function.
   /// \param key property key.
   /// \param value property value.
   void luaWriteProperty(const std::string& key, const std::string& value);
   /// \brief notifies lua that the player picked up an extra mechanism.
   /// \param extra_name extra identifier.
   void luaPlayerReceivedExtra(const std::string& extra_name);
   /// \brief notifies lua that an inventory item was added.
   /// \param item inventory item id.
   void luaPlayerReceivedItem(const std::string& item);
   /// \brief asks lua whether an inventory item use should be accepted.
   /// \param item inventory item id.
   /// \return boolean result returned by the lua playerUsedItem callback.
   bool luaPlayerUsedItem(const std::string& item);
   /// \brief notifies lua that the player touched a registered collision rectangle.
   /// \param rect_id collision rectangle id returned by addCollisionRect.
   void luaPlayerCollidesWithRect(int32_t rect_id);
   /// \brief notifies lua that the player entered a sensor rectangle mechanism.
   /// \param sensor_rect_id sensor rect object id.
   void luaPlayerCollidesWithSensorRect(const std::string& sensor_rect_id);
   /// \brief forwards mechanism enabled-state changes observed by the engine to lua.
   /// \param object_id mechanism object id.
   /// \param group mechanism group id.
   /// \param enabled updated enabled state.
   void luaMechanismEnabled(const std::string& object_id, const std::string& group, bool enabled);
   using LuaVariant = std::variant<std::string, int64_t, double, bool>;
   /// \brief forwards mechanism event payloads observed by the engine to lua.
   /// \param object_id mechanism object id.
   /// \param group mechanism group id.
   /// \param event_name event name emitted by the mechanism.
   /// \param value event payload variant value.
   void luaMechanismEvent(const std::string& object_id, const std::string& group, const std::string& event_name, const LuaVariant& value);

   using SearchMechanismCallback =
      std::function<std::vector<std::shared_ptr<GameMechanism>>(const std::string& regexp, const std::optional<std::string>&)>;

   /// \brief sets the mechanism lookup callback used by regex-based mechanism operations.
   /// \param callback function that returns matching mechanisms.
   void setSearchMechanismCallback(const SearchMechanismCallback& callback);
   /// \brief subscribes extra mechanisms so pickups trigger luaPlayerReceivedExtra.
   /// \param extras list of extra mechanisms in the level.
   void createExtraCallbacks(const std::vector<std::shared_ptr<GameMechanism>>& extras);

private:
   /// \brief closes the lua state if it is active.
   void stopScript();
   /// \brief finds LuaNode instances whose names fully match a regex.
   /// \param search_pattern regex used against LuaNode names.
   /// \return list of matching LuaNode instances.
   std::vector<std::shared_ptr<LuaNode>> findLuaNodes(const std::string& search_pattern);

   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};

   SearchMechanismCallback _search_mechanism_callback{nullptr};

   using ItemAddedCallback = std::function<void(const std::string&)>;
   using ItemUsedCallback = std::function<bool(const std::string&)>;
   ItemAddedCallback _inventory_added_callback;
   ItemUsedCallback _inventory_used_callback;

   std::unique_ptr<
      GameMechanismObserver::Reference<GameMechanismObserver::EnabledCallback>,
      std::function<void(GameMechanismObserver::Reference<GameMechanismObserver::EnabledCallback>*)>>
      _enabled_observer_reference;

   std::unique_ptr<
      GameMechanismObserver::Reference<GameMechanismObserver::EventCallback>,
      std::function<void(GameMechanismObserver::Reference<GameMechanismObserver::EventCallback>*)>>
      _event_observer_reference;
};
