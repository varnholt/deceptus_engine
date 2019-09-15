// base
#include "luanode.h"

// lua
#include "lua/lua.hpp"

// stl
#include <iostream>
#include <sstream>
#include <thread>

// game
#include "audio.h"
#include "constants.h"
#include "level.h"
#include "fixturenode.h"
#include "player.h"
#include "luaconstants.h"
#include "luainterface.h"
#include "timer.h"

// static
std::atomic<int32_t> LuaNode::sNextId = 0;

namespace  {
   uint16_t categoryBits = CategoryEnemyWalkThrough; // I am a ...
   uint16_t maskBits = CategoryBoundary;             // I collide with ...
   int16_t groupIndex = 0;                           // 0 is default
   // int16_t groupIndex = -1; // 0 is default
}


#define OBJINSTANCE LuaInterface::instance()->getObject(state)


extern "C" int32_t updateProperties(lua_State* state)
{
   lua_pushnil(state);

   while(lua_next(state, -2) != 0)
   {
      std::string key = lua_tostring(state, -2);

      if (lua_isboolean(state, -1)) // bool
      {
         OBJINSTANCE->mProperties[key] = static_cast<bool>(lua_toboolean(state, -1));
         // printf("%s = %d\n", key.c_str(), lua_toboolean(state, -1));
      }
      if (lua_isnumber(state, -1))
      {
         if (lua_isinteger(state, -1)) // int64
         {
            OBJINSTANCE->mProperties[key] = static_cast<int64_t>(lua_tointeger(state, -1));
            // printf("%s = %lld\n", key.c_str(), lua_tointeger(state, -1));
         }
         else // double
         {
            OBJINSTANCE->mProperties[key] = lua_tonumber(state, -1);
            // printf("%s = %f\n", key.c_str(), lua_tonumber(state, -1));
         }
      }
      else if (lua_isstring(state, -1)) // string
      {
         OBJINSTANCE->mProperties[key] = std::string(lua_tostring(state, -1));
         // printf("%s = %s\n", key.c_str(), lua_tostring(state, -1));
      }

      // process nested tables
      //
      //      else if(lua_istable(state, -1))
      //      {
      //         return updateProperties(state);
      //      }

      lua_pop(state, 1);
   }

   OBJINSTANCE->synchronizeProperties();

   return 0;
}


extern "C" int32_t updateSpriteRect(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 4)
   {
      auto x = static_cast<int32_t>(lua_tointeger(state, 1));
      auto y = static_cast<int32_t>(lua_tointeger(state, 2));
      auto w = static_cast<int32_t>(lua_tointeger(state, 3));
      auto h = static_cast<int32_t>(lua_tointeger(state, 4));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->updateSpriteRect(x, y, w, h);
   }

   return 0;
}



extern "C" int32_t makeDynamic(lua_State* state)
{
   auto node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->makeDynamic();
   return 0;
}


extern "C" int32_t makeStatic(lua_State* state)
{
   auto node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->makeStatic();
   return 0;
}


extern "C" int32_t setGravityScale(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 1)
   {

      auto scale = static_cast<float>(lua_tonumber(state, 1));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->setGravityScale(scale);
   }

   return 0;
}


extern "C" int32_t setActive(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 1)
   {

      auto active = static_cast<bool>(lua_toboolean(state, 1));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->setActive(active);
   }

   return 0;
}


extern "C" int32_t isPhsyicsPathClear(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 4)
   {
      // the lua scripts think in pixels; the physics grid has a resolution of 8x8 for each tile.
      // so that needs to be scaled first.
      auto x0 = static_cast<int32_t>(lua_tonumber(state, 1) / PIXELS_PER_PHYSICS_TILE);
      auto y0 = static_cast<int32_t>(lua_tonumber(state, 2) / PIXELS_PER_PHYSICS_TILE);
      auto x1 = static_cast<int32_t>(lua_tonumber(state, 3) / PIXELS_PER_PHYSICS_TILE);
      auto y1 = static_cast<int32_t>(lua_tonumber(state, 4) / PIXELS_PER_PHYSICS_TILE);

      // check map for collision
      auto collides = Level::getCurrentLevel()->isPhysicsPathClear({x0, y0}, {x1, y1});

      lua_pushboolean(state, !collides);
   }

   // 1 return value
   return 1;
}


extern "C" int32_t getLinearVelocity(lua_State* state)
{
   auto node = OBJINSTANCE;
   auto velocity = node->getLinearVelocity();

   lua_createtable(state, 2, 0);

   auto table = lua_gettop(state);
   auto index = 1;

   lua_pushnumber(state, static_cast<double>(velocity.x));
   lua_rawseti(state, table, index++);
   lua_pushnumber(state, static_cast<double>(velocity.y));
   lua_rawseti(state, table, index++);

   return 1;
}


extern "C" int32_t damage(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto damage = static_cast<int32_t>(lua_tonumber(state, 2));
      auto dx = static_cast<float>(lua_tonumber(state, 3));
      auto dy = static_cast<float>(lua_tonumber(state, 4));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->damage(damage, dx, dy);
   }

   return 0;
}


extern "C" int32_t setTransform(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto x = static_cast<float>(lua_tonumber(state, 1));
      auto y = static_cast<float>(lua_tonumber(state, 2));
      auto angle = static_cast<int32_t>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      b2Vec2 pos{x / PPM, y / PPM};
      node->setTransform(pos, angle);
   }

   return 0;
}



extern "C" int32_t boom(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto x = static_cast<float>(lua_tonumber(state, 1));
      auto y = static_cast<float>(lua_tonumber(state, 2));
      auto intensity = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->boom(x, y, intensity);
   }

   return 0;
}


extern "C" int32_t addShapeCircle(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 3)
   {
      auto r = static_cast<float>(lua_tonumber(state, 1));
      auto x = static_cast<float>(lua_tonumber(state, 2));
      auto y = static_cast<float>(lua_tonumber(state, 3));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->addShapeCircle(r, x, y);
   }

   return 0;
}


extern "C" int32_t addShapeRect(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 4)
   {
      auto width = static_cast<float>(lua_tonumber(state, 1));
      auto height = static_cast<float>(lua_tonumber(state, 2));
      auto x = static_cast<float>(lua_tonumber(state, 3));
      auto y = static_cast<float>(lua_tonumber(state, 4));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->addShapeRect(width, height, x, y);
   }

   return 0;
}


extern "C" int32_t addShapePoly(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc >= 2 && (argc % 2 == 0))
   {
      auto size = argc / 2;
      b2Vec2* poly = new b2Vec2[static_cast<uint32_t>(size)];
      auto polyIndex = 0;
      for (auto i = 0; i < argc; i += 2)
      {
         auto x = static_cast<float>(lua_tonumber(state, i));
         auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[polyIndex].Set(x, y);
         polyIndex++;
      }

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->addShapePoly(poly, size);
   }

   return 0;
}


extern "C" int32_t addWeapon(lua_State* state)
{
   auto argc = static_cast<size_t>(lua_gettop(state));
   auto fireInterval = 0;
   std::unique_ptr<b2Shape> shape;

   // add weapon with bullet radius only
   if (argc == 2)
   {
      fireInterval = static_cast<int>(lua_tointeger(state, 1));
      auto radius = static_cast<float>(lua_tonumber(state, 2));
      shape = std::make_unique<b2CircleShape>();
      dynamic_cast<b2CircleShape*>(shape.get())->m_radius = radius;
   }

   // add weapon with polygon bullet shape
   if (argc >= 3 && ((argc + 1) % 2 == 0))
   {
      fireInterval = static_cast<int>(lua_tointeger(state, 1));
      shape = std::make_unique<b2PolygonShape>();

      auto size = argc / 2;
      b2Vec2* poly = new b2Vec2[size];
      auto polyIndex = 0;
      for (auto i = 2; i < argc; i += 2)
      {
         auto x = static_cast<float>(lua_tonumber(state, i));
         auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[polyIndex].Set(x, y);
         polyIndex++;
      }

      dynamic_cast<b2PolygonShape*>(shape.get())->Set(poly, polyIndex);
   }

   std::shared_ptr<LuaNode> node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->addWeapon(std::move(shape), fireInterval);

   return 0;
}


extern "C" int32_t fireWeapon(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 5)
   {
      auto index = static_cast<size_t>(lua_tointeger(state, 1));

      auto posX = static_cast<float>(lua_tonumber(state, 2)) * MPP;
      auto posY = static_cast<float>(lua_tonumber(state, 3)) * MPP;

      auto dirX = static_cast<float>(lua_tonumber(state, 4));
      auto dirY = static_cast<float>(lua_tonumber(state, 5));

      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->fireWeapon(index, {posX, posY}, {dirX, dirY});
   }

   return 0;
}


extern "C" int32_t updateBulletTexture(lua_State* state)
{
   auto argc = lua_gettop(state);
   auto valid = (argc >= 2);

   auto index = 0u;
   std::string path;

   if (valid)
   {
      index = static_cast<uint32_t>(lua_tointeger(state, 1));
      path = lua_tostring(state, 2);
   }

   sf::Rect<int32_t> rect;
   if (argc == 6)
   {
      auto x1 = static_cast<int32_t>(lua_tointeger(state, 3));
      auto y1 = static_cast<int32_t>(lua_tointeger(state, 4));

      auto width = static_cast<int32_t>(lua_tointeger(state, 5));
      auto height = static_cast<int32_t>(lua_tointeger(state, 6));

      rect.left = x1;
      rect.top = y1;
      rect.width = width;
      rect.height = height;
   }


   if (valid)
   {
      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      node->mWeapons[index]->setTexture(path, rect);
   }

   return 0;
}



extern "C" int32_t timer(lua_State* state)
{
   // number of function arguments are on top of the stack.
   auto argc = lua_gettop(state);

   if (argc == 2)
   {
      auto delay = static_cast<int32_t>(lua_tointeger(state, 1));
      auto timerId = static_cast<int32_t>(lua_tointeger(state, 2));
      std::shared_ptr<LuaNode> node = OBJINSTANCE;

      if (!node)
      {
         return 0;
      }

      Timer::add(
         std::chrono::milliseconds(delay),
         [node, timerId](){node->luaTimeout(timerId);}
      );

      //      std::thread([node, delay, timerId]() {
      //            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      //            node->luaTimeout(timerId);
      //         }
      //      ).detach();
   }

   return 0;
}


extern "C" int32_t addSample(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 1)
   {
      auto sample = lua_tostring(state, 1);
      Audio::getInstance()->addSample(sample);
   }

   return 0;
}


extern "C" int32_t playSample(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 2)
   {
      auto sample = lua_tostring(state, 1);
      auto volume = static_cast<float>(lua_tonumber(state, 2));

      Audio::getInstance()->playSample(sample, volume);
   }

   return 0;
}


extern "C" int32_t debug(lua_State* state)
{
   // number of function arguments are on top of the stack.
   int32_t argc = lua_gettop(state);

   if (argc == 1)
   {
      const char* message = lua_tostring(state, 1);
      puts(message);
   }

   return 0;
}


void error(lua_State* state, const char* /*scope*/ = nullptr)
{
  // the error message is on top of the stack.
  // fetch it, print32_t it and then pop it off the stack.
   std::stringstream os;
   os << lua_tostring(state, -1);

   printf("%s\n", os.str().c_str());

   lua_pop(state, 1);

   exit(1);
}


extern "C" int32_t updateKeysPressed(lua_State* state)
{
   auto argc = lua_gettop(state);

   if (argc == 1)
   {
      auto keyPressed = static_cast<int32_t>(lua_tointeger(state, 1));

      auto obj = LuaInterface::instance()->getObject(state);
      if (obj != nullptr)
      {
         LuaInterface::instance()->updateKeysPressed(obj, keyPressed);
      }
   }

   return 0;
}


extern "C" int32_t requestMap(lua_State* state)
{
   auto obj = LuaInterface::instance()->getObject(state);
   if (obj != nullptr)
   {
      LuaInterface::instance()->requestMap(obj);
   }

   return 0;
}


extern "C" int32_t die(lua_State* state)
{
   std::shared_ptr<LuaNode> node = OBJINSTANCE;

   if (!node)
   {
      return 0;
   }

   node->luaDie();
   return 0;
}


void LuaNode::setupTexture()
{
   std::string spriteName = std::get<std::string>(mProperties["sprite"]);
   if (mTexture.loadFromFile(spriteName))
   {
      // mSprite.scale(4.0f, 4.0f);
      mSprite.setTexture(mTexture);
   }
   else
   {
      printf("failed loading object spriteset");
   }
}


LuaNode::LuaNode(const std::string &filename)
 : GameNode(nullptr),
   mScriptName(filename)
{
}


LuaNode::~LuaNode()
{
}


void LuaNode::deserializeEnemyDescription()
{
   std::vector<sf::Vector2f> patrolPath;
   for (auto i = 0u; i < mEnemyDescription.mPatrolPath.size(); i+= 2)
   {
      patrolPath.push_back(
         sf::Vector2f(
            static_cast<float_t>(mEnemyDescription.mPatrolPath.at(i)     * PIXELS_PER_TILE + PIXELS_PER_TILE / 2),
            static_cast<float_t>(mEnemyDescription.mPatrolPath.at(i + 1) * PIXELS_PER_TILE)
         )
      );
   }

   mStartPosition = sf::Vector2f(
      static_cast<float_t>(mEnemyDescription.mStartPosition.at(0) * PIXELS_PER_TILE + PIXELS_PER_TILE / 2),
      static_cast<float_t>(mEnemyDescription.mStartPosition.at(1) * PIXELS_PER_TILE + PIXELS_PER_TILE / 2)
   );

   mPatrolPath = patrolPath;
}


void LuaNode::initialize()
{
   deserializeEnemyDescription();
   setupLua();
   createBody();
}


void LuaNode::setupLua()
{
   mState = luaL_newstate();

   // register callbacks
   lua_register(mState, "addSample", ::addSample);
   lua_register(mState, "addShapeCircle", ::addShapeCircle);
   lua_register(mState, "addShapeRect", ::addShapeRect);
   lua_register(mState, "addWeapon", ::addWeapon);
   lua_register(mState, "boom", ::boom);
   lua_register(mState, "damage", ::damage);
   lua_register(mState, "debug", ::debug);
   lua_register(mState, "die", ::die);
   lua_register(mState, "fireWeapon", ::fireWeapon);
   lua_register(mState, "getLinearVelocity", ::getLinearVelocity);
   lua_register(mState, "playSample", ::playSample);
   lua_register(mState, "isPhsyicsPathClear", ::isPhsyicsPathClear);
   lua_register(mState, "makeDynamic", ::makeDynamic);
   lua_register(mState, "makeStatic", ::makeStatic);
   lua_register(mState, "setActive", ::setActive);
   lua_register(mState, "setGravityScale", ::setGravityScale);
   lua_register(mState, "setTransform", ::setTransform);
   lua_register(mState, "timer", ::timer);
   lua_register(mState, "updateBulletTexture", ::updateBulletTexture);
   lua_register(mState, "updateKeysPressed", ::updateKeysPressed);
   lua_register(mState, "updateProperties", ::updateProperties);
   lua_register(mState, "updateSpriteRect", ::updateSpriteRect);

   // make standard libraries available in the Lua object
   luaL_openlibs(mState);

   // load program
   auto result = luaL_loadfile(mState, mScriptName.c_str());
   if (result == LUA_OK)
   {
      // execute program
      result = lua_pcall(mState, 0, LUA_MULTRET, 0);

      if (result != LUA_OK)
      {
         error(mState);
      }
      else
      {
         luaMovedTo();
         luaInitialize();
         luaRetrieveProperties();
         luaSendPatrolPath();
      }
   }
   else
   {
      error(mState);
   }

   // register properties
   for (auto& prop : mEnemyDescription.mProperties)
   {
      luaWriteProperty(prop.mName, prop.mValue);
   }
}


void LuaNode::synchronizeProperties()
{
   // evaluate property map
   //
   //   int32_t i = std::get<int>(variant);
   //   w = std::get<int>(variant);
   //   w = std::get<0>(variant);

   // as soon as the texture is known, it can be set up
   setupTexture();
}


void LuaNode::luaMovedTo()
{
   const auto x = mPosition.x;
   const auto y = mPosition.y;

   lua_getglobal(mState, FUNCTION_MOVED_TO);

   lua_pushnumber(mState, static_cast<double>(x));
   lua_pushnumber(mState, static_cast<double>(y));

   // 3 args, 0 result
   auto result = lua_pcall(mState, 2, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_MOVED_TO);
   }
}


void LuaNode::luaPlayerMovedTo()
{
   const auto pos =  Player::getCurrent()->getPixelPosition();

   lua_getglobal(mState, FUNCTION_PLAYER_MOVED_TO);

   lua_pushnumber(mState, pos.x);
   lua_pushnumber(mState, pos.y);

   // 3 args, 0 result
   auto result = lua_pcall(mState, 2, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_PLAYER_MOVED_TO);
   }
}


void LuaNode::luaRetrieveProperties()
{
   lua_getglobal(mState, FUNCTION_RETRIEVE_PROPERTIES);

   // 0 args, 0 result
   auto result = lua_pcall(mState, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_RETRIEVE_PROPERTIES);
   }
}


void LuaNode::luaTimeout(int32_t timerId)
{
   lua_getglobal(mState, FUNCTION_TIMEOUT);
   lua_pushinteger(mState, timerId);

   auto result = lua_pcall(mState, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_TIMEOUT);
   }
}


void LuaNode::luaSendPath(const std::vector<sf::Vector2f>& vec)
{
   lua_newtable(mState);

   int32_t i = 0;
   for (const auto& v : vec)
   {
      lua_pushnumber(mState, v.x); // push x
      lua_rawseti(mState,-2,++i);
      lua_pushnumber(mState, v.y); // push y
      lua_rawseti(mState,-2,++i);
   }
}


void LuaNode::damage(int32_t damage, float forceX, float forceY)
{
   Player::getCurrent()->damage(damage, sf::Vector2f(forceX, forceY));
}


b2Vec2 LuaNode::getLinearVelocity() const
{
   b2Vec2 velocity;

   if (mBody)
   {
      velocity = mBody->GetLinearVelocity();
   }

   return velocity;
}


void LuaNode::boom(float x, float y, float intensity)
{
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, intensity);
}


void LuaNode::setGravityScale(float scale)
{
   mBody->SetGravityScale(scale);
}

void LuaNode::setTransform(const b2Vec2& position, float32 angle)
{
   mBody->SetTransform(position, angle);
}


void LuaNode::setActive(bool active)
{
   mBody->SetActive(active);
}


void LuaNode::makeDynamic()
{
   mBody->SetType(b2_dynamicBody);
}


void LuaNode::makeStatic()
{
   mBody->SetType(b2_staticBody);
}


void LuaNode::luaSendPatrolPath()
{
   if (mPatrolPath.size() == 0)
   {
      return;
   }

   lua_getglobal(mState, FUNCTION_SET_PATH);

   lua_pushstring(mState, "patrol_path");
   luaSendPath(mPatrolPath);

   // vec.size + 1 args, 0 result
   auto result = lua_pcall(mState, 2, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_SET_PATH);
   }
}


void LuaNode::luaDie()
{
   Level::getCurrentLevel()->getWorld()->DestroyBody(mBody);
   mBody = nullptr;
}


bool LuaNode::getPropertyBool(const std::string& key)
{
   auto value = false;
   auto it = mProperties.find(key);
   if (it !=  mProperties.end())
      value = std::get<bool>(it->second);
   return value;
}


double LuaNode::getPropertyDouble(const std::string& key)
{
   auto value = 0.0;
   auto it = mProperties.find(key);
   if (it !=  mProperties.end())
      value = std::get<double>(it->second);
   return value;
}


int64_t LuaNode::getPropertyInt64(const std::string &key)
{
   auto value = 0LL;
   auto it = mProperties.find(key);
   if (it !=  mProperties.end())
      value = std::get<int64_t>(it->second);
   return value;
}


void LuaNode::createBody()
{
   auto staticBody = getPropertyBool("staticBody");
   auto damage = static_cast<int32_t>(getPropertyInt64("damage"));
   auto sensor = static_cast<bool>(getPropertyBool("sensor"));

   mBodyDef = new b2BodyDef();
   mBodyDef->type = staticBody ? b2_staticBody : b2_dynamicBody;
   mBodyDef->position.Set(
      mStartPosition.x * MPP,
      mStartPosition.y * MPP
   );

   mBody = Level::getCurrentLevel()->getWorld()->CreateBody(mBodyDef);
   mBody->SetFixedRotation(true);

   for (auto shape : mShapes)
   {
      b2FixtureDef fd;
      fd.density = 1.f;
      fd.friction = 0.0f;
      fd.restitution = 0.0f;
      fd.shape = shape;

      // apply default filter
      // http://www.iforce2d.net/b2dtut/collision-filtering
      fd.filter.groupIndex = groupIndex;
      fd.filter.maskBits = maskBits;
      fd.filter.categoryBits = categoryBits;

      b2Fixture* ft = mBody->CreateFixture(&fd);
      FixtureNode* fn = new FixtureNode(this);
      fn->setType(ObjectTypeEnemy);
      fn->setProperty("damage", damage);
      ft->SetUserData(static_cast<void*>(fn));

      if (sensor)
      {
         ft->SetSensor(true);
      }
   }
}


void LuaNode::addShapeCircle(float radius, float x, float y)
{
   b2CircleShape* shape = new b2CircleShape();
   shape->m_p.Set(x, y);
   shape->m_radius = radius;
   mShapes.push_back(shape);
}


void LuaNode::addShapeRect(float width, float height, float x, float y)
{
   b2PolygonShape* shape = new b2PolygonShape();
   shape->SetAsBox(width, height, b2Vec2(x, y), 0.0f);
   mShapes.push_back(shape);
}


void LuaNode::addShapePoly(const b2Vec2* points, int32_t size)
{
   b2PolygonShape* shape = new b2PolygonShape();
   shape->Set(points, size);
   mShapes.push_back(shape);
}


void LuaNode::addWeapon(std::unique_ptr<b2Shape> shape, int32_t fireInterval)
{
   auto weapon = std::make_unique<Weapon>(std::move(shape), fireInterval);
   mWeapons.push_back(std::move(weapon));
}


void LuaNode::fireWeapon(size_t index, b2Vec2 from, b2Vec2 to)
{
   mWeapons[index]->fireInIntervals(Level::getCurrentLevel()->getWorld(), from, to);
}


void LuaNode::luaInitialize()
{
   lua_getglobal(mState, FUNCTION_INITIALIZE);
   auto result = lua_pcall(mState, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_INITIALIZE);
   }
}


void LuaNode::luaUpdate(const sf::Time& dt)
{
   lua_getglobal(mState, FUNCTION_UPDATE);
   lua_pushnumber(mState, dt.asSeconds());

   auto result = lua_pcall(mState, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(mState, FUNCTION_UPDATE);
   }
}

void LuaNode::luaWriteProperty(const std::string& key, const std::string& value)
{
   lua_getglobal(mState, FUNCTION_WRITE_PROPERTY);
   if (lua_isfunction(mState, -1) )
   {
      lua_pushstring(mState, key.c_str());
      lua_pushstring(mState, value.c_str());

      auto result = lua_pcall(mState, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(mState, FUNCTION_UPDATE);
      }
   }
}


void LuaNode::stopScript()
{
   if (mState != nullptr)
   {
      lua_close(mState);
      mState = nullptr;

      printf("LuaInterface::StopScript: script stopped\n");
   }
}


const EnemyDescription& LuaNode::getEnemyDescription() const
{
   return mEnemyDescription;
}


void LuaNode::setEnemyDescription(const EnemyDescription& enemyDescription)
{
   mEnemyDescription = enemyDescription;
}


void LuaNode::updateVelocity()
{
   if (!mBody)
   {
      return;
   }

   auto velocityMax = 0.0;
   auto acceleration = 0.0;

   auto velIt = mProperties.find("velocity_walk_max");
   if (velIt != mProperties.end())
   {
      velocityMax = *std::get_if<double>(&(velIt->second));
   }

   auto accIt = mProperties.find("acceleration_ground");
   if (accIt != mProperties.end())
   {
      acceleration = *std::get_if<double>(&(accIt->second));
   }

   auto desiredVel = 0.0f;
   auto velocity = mBody->GetLinearVelocity();

   if (mKeysPressed & KeyPressedLeft)
   {
      desiredVel = static_cast<float>(b2Max(velocity.x - acceleration, -velocityMax));
   }

   if (mKeysPressed & KeyPressedRight)
   {
      desiredVel = static_cast<float>(b2Min( velocity.x + acceleration, velocityMax));
   }

   // calc impulse, disregard time factor
   auto velChange = desiredVel - velocity.x;
   auto impulse = mBody->GetMass() * velChange;

   mBody->ApplyLinearImpulse(
      b2Vec2(impulse, 0.0f),
      mBody->GetWorldCenter(),
      true
   );
}


void LuaNode::updatePosition()
{
   if (!mBody)
   {
      return;
   }

   auto x = mBody->GetPosition().x * PPM;
   auto y = mBody->GetPosition().y * PPM;

   mPosition.x = x;
   mPosition.y = y;
}


void LuaNode::updateSpriteRect(int32_t x, int32_t y, int32_t w, int32_t h)
{
   mSpriteOffset.x = static_cast<uint32_t>(x);
   mSpriteOffset.y = static_cast<uint32_t>(y);
   mSpriteWidth = w;
   mSpriteHeight = h;

   mSprite.setTextureRect(
      sf::IntRect(
         x,
         y,
         mSpriteWidth,
         mSpriteHeight
      )
   );
}


void LuaNode::draw(sf::RenderTarget& target)
{
   /*
   if (!mVisible)
   {
      return;
   }

   sf::Time time = mClock.getElapsedTime();

   if (time.asMilliseconds() >= mAnimSpeed)
   {
      mSpriteAnim.x++;

      if ((mSpriteAnim.x * SPRITE_WIDTH) >= mTexture.getSize().x)
         mSpriteAnim.x = 0;

      mClock.restart();
   }
   */

   mSprite.setPosition(mPosition - sf::Vector2f(mSpriteWidth / 2.0f, mSpriteHeight / 2.0f));
   target.draw(mSprite);

   for (auto& w : mWeapons)
   {
      w->drawBullets(target);
   }
}


