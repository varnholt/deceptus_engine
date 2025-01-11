#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "SFML/Graphics.hpp"

#include "game/level/luanode.h"

class LuaInterface
{
public:
   static LuaInterface& instance();

   using ChunkFilter = std::function<bool(const std::shared_ptr<GameMechanism>&)>;

   void initialize();
   void update(const sf::Time& dt, const ChunkFilter& filter);
   void reset();

   std::shared_ptr<LuaNode> addObject(GameNode* parent, const std::string& filename);
   std::shared_ptr<LuaNode> getObject(lua_State*);
   const std::vector<std::shared_ptr<LuaNode>>& getObjectList();

private:
   LuaInterface() = default;
};
