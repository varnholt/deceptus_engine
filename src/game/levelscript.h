#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <string>
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
   void setMechanismEnabled(const std::string& mechanism_id, bool enabled);
   bool isMechanismEnabled(const std::string& mechanism_id) const;
   void addSkill(int32_t skill);
   void removeSkill(int32_t skill);

   // functions on the lua end
   void luaInitialize();
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaPlayerReceivedExtra(const std::string& extra_name);

private:
   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};
};
