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

private:
   // functions on the lua end
   void luaInitialize();
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaPlayerReceivedExtra(const std::string& extra_name);

   std::vector<sf::IntRect> _collision_rects;
   std::vector<ScriptProperty> _properties;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};
};
