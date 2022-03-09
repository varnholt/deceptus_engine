#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <string>
#include "lua/lua.hpp"


class LevelScript
{

public:

   LevelScript() = default;
   void setup(const std::filesystem::path& path);
   void update(const sf::Time& dt);


private:

   // functions on the lua end
   void luaInitialize();
   void luaUpdate(const sf::Time& dt);

   std::string _script_name;
   lua_State* _lua_state = nullptr;
};

