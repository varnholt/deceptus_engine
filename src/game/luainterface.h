#pragma once


#include <memory>
#include <string>
#include <vector>

#include "SFML/Graphics.hpp"

#include "luanode.h"


class LuaInterface
{

public:

   static LuaInterface* instance();

   std::shared_ptr<LuaNode> addObject(const std::string &filename);
   void removeObject(const std::shared_ptr<LuaNode>& node);

   void initialize();

   void update(const sf::Time& dt);

   void requestMap(std::shared_ptr<LuaNode> obj);

   void updateKeysPressed(std::shared_ptr<LuaNode> obj, int keys);

   void reset();

   std::shared_ptr<LuaNode> getObject(lua_State*);


private:

   explicit LuaInterface();

   static LuaInterface* __instance;
   std::vector<std::shared_ptr<LuaNode>> _object_list;
};

