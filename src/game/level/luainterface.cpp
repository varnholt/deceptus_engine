#include "game/level/luainterface.h"

// lua
#include "lua.hpp"

// game
#include "framework/tools/log.h"

// stl
#include <iostream>
#include <sstream>

namespace
{
std::vector<std::shared_ptr<LuaNode>> _object_list;

std::vector<std::shared_ptr<LuaNode>>::iterator removeObject(const std::shared_ptr<LuaNode>& node)
{
   const auto it = _object_list.erase(std::remove(_object_list.begin(), _object_list.end(), node), _object_list.end());

   if (node.use_count() > 1)
   {
      Log::Warning() << node->_script_name << " use count is: " << node.use_count() << " address: " << node.get();
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

void LuaInterface::update(const sf::Time& dt, const ChunkFilter& filter)
{
   for (auto it = _object_list.begin(); it != _object_list.end();)
   {
      {
         const auto& object = *it;

         if (!filter(object))
         {
            ++it;
            continue;
         }

         object->luaMovedTo();
         object->luaPlayerMovedTo();
         object->luaUpdate(dt);
         object->updateVelocity();
         object->updatePosition();
         object->updateWeapons(dt);
      }

      if ((*it)->_dead)
      {
         it = removeObject(*it);
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
