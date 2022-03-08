#include "luainterface.h"

// lua
#include "lua/lua.hpp"

// stl
#include <sstream>


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


void LuaInterface::removeObject(const std::shared_ptr<LuaNode>& node)
{
   _object_list.erase(std::remove(_object_list.begin(), _object_list.end(), node), _object_list.end());
}


void LuaInterface::update(const sf::Time& dt)
{
   for (auto it = _object_list.begin(); it != _object_list.end();)
   {
      auto object = *it;

      object->luaMovedTo();
      object->luaPlayerMovedTo();
      object->luaUpdate(dt);
      object->updateVelocity();
      object->updatePosition();
      object->updateWeapons(dt);

      if (!object->_body)
      {
         it = _object_list.erase(it);
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

   std::vector<std::shared_ptr<LuaNode>>::iterator it =
      std::find_if(
         _object_list.begin(),
         _object_list.end(),
         [state](auto node) { return node->_lua_state == state; }
      );

   if (it != _object_list.end())
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



