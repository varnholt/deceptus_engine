#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <string>
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

   std::vector<sf::IntRect> _collision_rects;

   std::string _script_name;
   lua_State* _lua_state = nullptr;
   bool _initialized{false};
};
