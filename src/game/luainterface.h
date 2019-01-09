#ifndef LUAINTERFACE_H
#define LUAINTERFACE_H

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

   void initialize();

   void update(float dt);

   void requestMap(std::shared_ptr<LuaNode> obj);

   void updateKeysPressed(std::shared_ptr<LuaNode> obj, int keys);

   std::shared_ptr<LuaNode> getObject(lua_State*);


private:

   explicit LuaInterface();

   static LuaInterface* sInstance;

   std::vector<std::shared_ptr<LuaNode>> mObjectList;
};

#endif // LUAINTERFACE_H
