#include "luainterface.h"

// lua
#include "lua/lua.hpp"

// stl
#include <iostream>
#include <sstream>

namespace
{
std::vector<std::shared_ptr<LuaNode>>::iterator
removeObject(const std::shared_ptr<LuaNode>& node, std::vector<std::shared_ptr<LuaNode>>& object_list)
{
   const auto it = object_list.erase(std::remove(object_list.begin(), object_list.end(), node), object_list.end());

   if (node.use_count() > 1)
   {
      std::cout << "you fucked up. fix your code" << std::endl;
      exit(0);
   }

   return it;
}

}  // namespace

LuaInterface& LuaInterface::instance()
{
   static LuaInterface __instance;
   return __instance;
}

std::shared_ptr<LuaNode> LuaInterface::addObject(GameNode* parent, const std::string& filename)
{
   std::shared_ptr<LuaNode> object = std::make_shared<LuaNode>(parent, filename);
   _object_list.push_back(object);
   return object;
}

void LuaInterface::update(const sf::Time& dt)
{
   for (auto it = _object_list.begin(); it != _object_list.end();)
   {
      {
         const auto& object = *it;
         object->luaMovedTo();
         object->luaPlayerMovedTo();
         object->luaUpdate(dt);
         object->updateVelocity();
         object->updatePosition();
         object->updateWeapons(dt);
      }

      if ((*it)->_dead)
      {
         it = removeObject((*it), _object_list);
      }
      else
      {
         ++it;
      }
   }
}

std::shared_ptr<LuaNode> LuaInterface::getObject(lua_State* state)
{
   auto obj = std::shared_ptr<LuaNode>{nullptr};

   const auto it =
      std::find_if(_object_list.cbegin(), _object_list.cend(), [state](const auto& node) { return node->_lua_state == state; });

   if (it != _object_list.cend())
   {
      obj = *it;
   }

   return obj;
}

const std::vector<std::shared_ptr<LuaNode>>& LuaInterface::getObjectList()
{
   return _object_list;
}

void LuaInterface::reset()
{
   _object_list.clear();
}
