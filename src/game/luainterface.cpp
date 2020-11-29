#include "luainterface.h"

// lua
#include "lua/lua.hpp"

// stl
#include <sstream>


LuaInterface* LuaInterface::sInstance = nullptr;



LuaInterface *LuaInterface::instance()
{
   if (sInstance == nullptr)
   {
      new LuaInterface();
   }

   return sInstance;
}


LuaInterface::LuaInterface()
{
   sInstance = this;
}


std::shared_ptr<LuaNode> LuaInterface::addObject(const std::string &filename)
{
   std::shared_ptr<LuaNode> object = std::make_shared<LuaNode>(filename);
   mObjectList.push_back(object);
   return object;
}


void LuaInterface::removeObject(const std::shared_ptr<LuaNode>& node)
{
   mObjectList.erase(std::remove(mObjectList.begin(), mObjectList.end(), node), mObjectList.end());
}


void LuaInterface::update(const sf::Time& dt)
{
   for (auto it = mObjectList.begin(); it != mObjectList.end();)
   {
      auto object = *it;

      object->luaMovedTo();
      object->luaPlayerMovedTo();
      object->luaUpdate(dt);
      object->updateVelocity();
      object->updatePosition();
      object->updateWeapons(dt);

      if (!object->mBody)
         it = mObjectList.erase(it);
      else
         ++it;
   }
}


std::shared_ptr<LuaNode> LuaInterface::getObject(lua_State* state)
{
   auto obj = std::shared_ptr<LuaNode>{nullptr};

   std::vector<std::shared_ptr<LuaNode>>::iterator it =
      std::find_if(
         mObjectList.begin(),
         mObjectList.end(),
         [state](auto node) { return node->mState == state; }
      );

   if (it != mObjectList.end())
   {
      obj = *it;
   }

   return obj;
}


void LuaInterface::requestMap(std::shared_ptr<LuaNode> obj)
{
   printf("requestMap: obj: %d\n", obj->mId);
}


void LuaInterface::updateKeysPressed(std::shared_ptr<LuaNode> obj, int keys)
{
   // printf("keyPressed: obj: %d, keys: %d\n", obj->mId, keys);
   obj->mKeysPressed = keys;
}


void LuaInterface::reset()
{
   mObjectList.clear();
}



