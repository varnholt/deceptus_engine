#pragma once


#include <memory>
#include <string>
#include <vector>

#include "SFML/Graphics.hpp"

#include "luanode.h"


class LuaInterface
{

public:

   static LuaInterface& instance();

   void initialize();
   void update(const sf::Time& dt);
   void reset();

   std::shared_ptr<LuaNode> addObject(GameNode* parent, const std::string &filename);
   std::shared_ptr<LuaNode> getObject(lua_State*);
   const std::vector<std::shared_ptr<LuaNode>>& getObjectList();


private:

   LuaInterface() = default;

   std::vector<std::shared_ptr<LuaNode>> _object_list;
};

