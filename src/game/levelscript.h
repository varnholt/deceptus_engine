#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <string>
#include "game/gamemechanism.h"
#include "game/scriptproperty.h"
#include "lua.hpp"

class LevelScript
{
public:
   LevelScript();
   ~LevelScript();
   void setup(const std::filesystem::path& path);
   void update(const sf::Time& dt);
   int32_t addCollisionRect(const sf::IntRect& rect);
   void setMechanismEnabled(const std::string& search_pattern, bool enabled, const std::optional<std::string>& group = std::nullopt);
   bool isMechanismEnabled(const std::string& mechanism_id, const std::optional<std::string>& group) const;
   void toggle(const std::string& search_pattern, const std::optional<std::string>& group = std::nullopt);
   void addPlayerSkill(int32_t skill);
   void removePlayerSkill(int32_t skill);

   void giveWeaponBow();
   void giveWeaponGun();
   void giveWeaponSword();

   // functions on the lua end
   void luaInitialize();
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaPlayerReceivedExtra(const std::string& extra_name);

   using SearchMechanismCallback =
      std::function<std::vector<std::shared_ptr<GameMechanism>>(const std::string& regexp, const std::optional<std::string>&)>;

   void setSearchMechanismCallback(const SearchMechanismCallback& callback);

private:
   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};
   SearchMechanismCallback _search_mechanism_callback{nullptr};
};
