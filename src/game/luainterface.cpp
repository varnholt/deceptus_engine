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


void LuaInterface::update(float dt)
{
   for (auto& object : mObjectList)
   {
      // object->updateAtmosphere();
      // object->updateFire();
      // object->updateJump(dt);
      // object->updatePlatformMovement(dt);
      // object->updateDeath();
      object->luaMovedTo();
      object->luaPlayerMovedTo();
      object->luaAct(dt);
      object->updateVelocity();
      object->updatePosition();
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



