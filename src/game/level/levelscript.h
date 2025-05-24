#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <lua.hpp>
#include <string>
#include <variant>

#include "game/level/scriptproperty.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/gamemechanismobserver.h"

struct LuaNode;

class LevelScript
{
public:
   LevelScript();
   ~LevelScript();

   void setup(const std::filesystem::path& path);
   void update(const sf::Time& dt);

   int32_t addCollisionRect(const sf::IntRect& rect);
   void addSensorRectCallback(const std::string& sensor_rect_id);
   void setMechanismEnabled(const std::string& search_pattern, bool enabled, const std::optional<std::string>& group = std::nullopt);
   bool isMechanismEnabled(const std::string& mechanism_id, const std::optional<std::string>& group) const;
   void setMechanismVisible(const std::string& search_pattern, bool visible, const std::optional<std::string>& group = std::nullopt);
   bool isMechanismVisible(const std::string& mechanism_id, const std::optional<std::string>& group) const;
   void toggle(const std::string& search_pattern, const std::optional<std::string>& group = std::nullopt);
   void addPlayerSkill(int32_t skill);
   void removePlayerSkill(int32_t skill);
   void addPlayerHealth(int32_t health_points_to_add);
   void addPlayerHealthMax(int32_t health_points_to_add);
   void giveWeaponBow();
   void giveWeaponGun();
   void giveWeaponSword();
   void writeLuaNodeProperty(const std::string& search_pattern, const std::string& key, const std::string& value);
   void setLuaNodeVisible(const std::string& search_pattern, bool visible);
   void setLuaNodeActive(const std::string& search_pattern, bool active);
   void showDialogue(const std::string& search_pattern);
   void lockPlayerControls(const std::chrono::milliseconds& duration);
   void setZoomFactor(float zoom_factor);

   // functions on the lua end
   void luaInitialize();
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaPlayerReceivedExtra(const std::string& extra_name);
   void luaPlayerReceivedItem(const std::string& item);
   void luaPlayerCollidesWithRect(int32_t rect_id);
   void luaPlayerCollidesWithSensorRect(const std::string& sensor_rect_id);
   void luaMechanismEnabled(const std::string& object_id, const std::string& group, bool enabled);
   using LuaVariant = std::variant<std::string, int64_t, double, bool>;
   void luaMechanismEvent(const std::string& object_id, const std::string& group, const std::string& event_name, const LuaVariant& value);

   using SearchMechanismCallback =
      std::function<std::vector<std::shared_ptr<GameMechanism>>(const std::string& regexp, const std::optional<std::string>&)>;

   void setSearchMechanismCallback(const SearchMechanismCallback& callback);
   void createExtraCallbacks(const std::vector<std::shared_ptr<GameMechanism>>& extras);

private:
   std::vector<std::shared_ptr<LuaNode>> findLuaNodes(const std::string& search_pattern);

   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};

   SearchMechanismCallback _search_mechanism_callback{nullptr};

   using ItemAddedCallback = std::function<void(const std::string&)>;
   ItemAddedCallback _added_callback;

   std::unique_ptr<
      GameMechanismObserver::Reference<GameMechanismObserver::EnabledCallback>,
      std::function<void(GameMechanismObserver::Reference<GameMechanismObserver::EnabledCallback>*)>>
      _enabled_observer_reference;

   std::unique_ptr<
      GameMechanismObserver::Reference<GameMechanismObserver::EventCallback>,
      std::function<void(GameMechanismObserver::Reference<GameMechanismObserver::EventCallback>*)>>
      _event_observer_reference;
};
