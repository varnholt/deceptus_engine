#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <lua.hpp>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "game/animation/animationpool.h"
#include "game/audio/musicplayertypes.h"
#include "game/level/scriptproperty.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/gamemechanismobserver.h"

struct FadeTransitionEffect;
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

   /// \brief triggers a colour flash on all matching RingShaderLayer mechanisms.
   /// \param search_pattern regex used to select mechanisms.
   /// \param red red component 0-1.
   /// \param green green component 0-1.
   /// \param blue blue component 0-1.
   /// \param duration_s fade-out duration in seconds.
   void flashMechanism(const std::string& search_pattern, float red, float green, float blue, float duration_s);

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

   /// \brief fades the screen to black and holds it there until fadeIn is called.
   /// \param speed fade speed in alpha-per-second (1.0 = one second for a full fade).
   void fadeOut(float speed);

   /// \brief fades the screen from black back to visible.
   /// \param speed fade speed in alpha-per-second.
   void fadeIn(float speed);

   /// \brief sets camera zoom factor.
   /// \param zoom_factor absolute zoom multiplier.
   void setZoomFactor(float zoom_factor);

   /// \brief sets the ambient light color for the current level.
   /// \param color rgba color (0–255 per channel).
   void setAmbient(sf::Color color);

   /// \brief marks an achievement as earned in the current save slot.
   /// \param identifier achievement identifier.
   void addAchievement(const std::string& identifier);

   /// \brief checks whether an achievement has been earned in the current save slot.
   /// \param identifier achievement identifier.
   /// \return true when the achievement is earned.
   bool hasAchievement(const std::string& identifier);

   /// \brief marks a treasure as collected in the current save slot.
   /// \param identifier treasure identifier.
   void addTreasure(const std::string& identifier);

   /// \brief checks whether a treasure has been collected in the current save slot.
   /// \param identifier treasure identifier.
   /// \return true when the treasure is collected.
   bool hasTreasure(const std::string& identifier);

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

   /// \brief snaps the camera to an arbitrary world pixel position and locks tracking.
   /// \param x_px target x in world pixels.
   /// \param y_px target y in world pixels.
   void setCameraPosition(float x_px, float y_px);

   /// \brief releases the camera lock set by setCameraPosition and resumes player tracking.
   void unlockCamera();

   /// \brief returns the camera center in world pixel coordinates.
   /// \return world-space center of the camera view.
   sf::Vector2f getCameraCenter() const;

   /// \brief returns the bounding box of the first mechanism that matches the query.
   /// \param search_pattern regex used to select mechanisms.
   /// \param group optional mechanism group filter.
   /// \return bounding rect in world pixels, or nullopt when no match is found.
   std::optional<sf::FloatRect> getMechanismRect(const std::string& search_pattern, const std::optional<std::string>& group = std::nullopt)
      const;

   /// \brief shows or hides the player sprite.
   /// \param visible false to suppress player rendering.
   void setPlayerVisible(bool visible);

   /// \brief shows or hides the hud layer.
   /// \param visible false to suppress hud rendering.
   void setHudVisible(bool visible);

   /// \brief triggers the next-level transition via the global callback map.
   void nextLevel();

   /// \brief plays a one-shot sound effect.
   /// \param sample_name audio sample identifier.
   void playSound(const std::string& sample_name);

   /// \brief creates a managed cutscene sprite and starts playing its animation.
   /// \param name unique sprite identifier used in subsequent operations.
   /// \param animation_file path to the animation settings json file.
   /// \param animation_id animation name as defined in the settings file.
   /// \param x_px initial x position in world pixels.
   /// \param y_px initial y position in world pixels.
   /// \param looped true to loop the animation continuously.
   void createSprite(
      const std::string& name,
      const std::string& animation_file,
      const std::string& animation_id,
      float x_px,
      float y_px,
      bool looped
   );

   /// \brief removes a managed cutscene sprite by name.
   /// \param name sprite identifier passed to createSprite.
   void destroySprite(const std::string& name);

   /// \brief switches the active animation on a managed cutscene sprite.
   /// \param name sprite identifier.
   /// \param animation_id animation name in the sprite's animation file.
   /// \param looped true to loop the animation continuously.
   void setSpriteAnimation(const std::string& name, const std::string& animation_id, bool looped);

   /// \brief shows or hides a managed cutscene sprite.
   /// \param name sprite identifier.
   /// \param visible false to hide the sprite without destroying it.
   void setSpriteVisible(const std::string& name, bool visible);

   /// \brief starts moving a managed sprite toward a target at a constant speed.
   /// \param name sprite identifier.
   /// \param target_x target x position in world pixels.
   /// \param target_y target y position in world pixels.
   /// \param speed_px_per_s movement speed in pixels per second.
   /// \param arrive_event event name fired via onEvent when the sprite reaches the target.
   void moveSpriteAtSpeed(const std::string& name, float target_x, float target_y, float speed_px_per_s, const std::string& arrive_event);

   /// \brief draws all managed cutscene sprites to the given render target.
   /// \param target render target that receives sprite output.
   void draw(sf::RenderTarget& target);

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

   /// \brief returns the currently active LevelScript instance.
   /// \return pointer to the active instance, or nullptr when none is active.
   static LevelScript* getCurrent();

private:
   struct CutsceneSprite
   {
      std::string _name;
      std::shared_ptr<AnimationPool> _pool;
      std::shared_ptr<Animation> _animation;
      sf::Vector2f _position;
      sf::Vector2f _target;
      float _speed_px_per_s{0.0f};
      std::string _arrive_event;
      bool _moving{false};
   };

   /// \brief closes the lua state if it is active.
   void stopScript();

   /// \brief finds LuaNode instances whose names fully match a regex.
   /// \param search_pattern regex used against LuaNode names.
   /// \return list of matching LuaNode instances.
   std::vector<std::shared_ptr<LuaNode>> findLuaNodes(const std::string& search_pattern);

   /// \brief advances all moving cutscene sprites toward their targets for one frame.
   /// \param dt elapsed frame time.
   void updateCutsceneSprites(const sf::Time& dt);

   /// \brief calls the lua onEvent function with the given event name.
   /// \param event_name event identifier delivered to onEvent.
   void luaRaiseEvent(const std::string& event_name);

   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::vector<CutsceneSprite> _cutscene_sprites;
   std::map<std::string, std::shared_ptr<AnimationPool>> _cutscene_pools;

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

   std::shared_ptr<FadeTransitionEffect> _pending_fade_in;
};
