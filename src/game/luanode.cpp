// base
#include "luanode.h"

// lua
#include "lua/lua.hpp"

// stl
#include <iostream>
#include <sstream>
#include <thread>

// game
#include "animationplayer.h"
#include "audio.h"
#include "constants.h"
#include "detonationanimation.h"
#include "fixturenode.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "gun.h"
#include "level.h"
#include "luaconstants.h"
#include "luainterface.h"
#include "player/player.h"
#include "texturepool.h"
#include "weaponfactory.h"

namespace
{
uint16_t category_bits = CategoryEnemyWalkThrough;                  // I am a ...
uint16_t mask_bits_standing = CategoryBoundary | CategoryFriendly;  // I collide with ...
int16_t group_index = 0;                                            // 0 is default

#define OBJINSTANCE LuaInterface::instance().getObject(state)

/**
 * @brief updateProperties
 * @param state lua state
 *    param 1 key
 *    param 2 value
 *    param n key
 *    param n + 1 value
 * @return error code
 */
int32_t updateProperties(lua_State* state)
{
   lua_pushnil(state);

   while (lua_next(state, -2) != 0)
   {
      std::string key = lua_tostring(state, -2);

      if (lua_isboolean(state, -1))  // bool
      {
         OBJINSTANCE->_properties[key] = static_cast<bool>(lua_toboolean(state, -1));
         // printf("%s = %d\n", key.c_str(), lua_toboolean(state, -1));
      }
      if (lua_isnumber(state, -1))
      {
         if (lua_isinteger(state, -1))  // int64
         {
            OBJINSTANCE->_properties[key] = static_cast<int64_t>(lua_tointeger(state, -1));
            // printf("%s = %lld\n", key.c_str(), lua_tointeger(state, -1));
         }
         else  // double
         {
            OBJINSTANCE->_properties[key] = lua_tonumber(state, -1);
            // printf("%s = %f\n", key.c_str(), lua_tonumber(state, -1));
         }
      }
      else if (lua_isstring(state, -1))  // string
      {
         OBJINSTANCE->_properties[key] = std::string(lua_tostring(state, -1));
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

/**
 * @brief addHitBox add a hitbox to the enemy
 * @param state lua state
 *    param 1: x position of sprite
 *    param 2: y position of sprite
 *    param 3: sprite width
 *    param 4: sprite height
 * @return error code
 */
int32_t addHitbox(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x_px = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto y_px = static_cast<int32_t>(lua_tointeger(state, 2));
   const auto w_px = static_cast<int32_t>(lua_tointeger(state, 3));
   const auto h_px = static_cast<int32_t>(lua_tointeger(state, 4));
   node->addHitbox(x_px, y_px, w_px, h_px);

   return 0;
}

/**
 * @brief updateSpriteRect update node's sprite rect
 * @param state lua state
 *    param 1: id of sprite
 *    param 2: x position of sprite
 *    param 3: y position of sprite
 *    param 4: sprite width
 *    param 5: sprite height
 * @return error code
 */
int32_t updateSpriteRect(lua_State* state)
{
   const auto argc = lua_gettop(state);

   if (argc != 5)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto id = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto x_px = static_cast<int32_t>(lua_tointeger(state, 2));
   const auto y_px = static_cast<int32_t>(lua_tointeger(state, 3));
   const auto w_px = static_cast<int32_t>(lua_tointeger(state, 4));
   const auto h_px = static_cast<int32_t>(lua_tointeger(state, 5));
   node->updateSpriteRect(id, x_px, y_px, w_px, h_px);

   return 0;
}

/**
 * @brief setSpriteColor change a sprite's color
 * @param state lua state
 *    param 1: id of sprite
 *    param 2: r part in uint8
 *    param 3: g part in uint8
 *    param 4: b part in uint8
 *    param 5: a part in uint8
 * @return error code
 */
int32_t setSpriteColor(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 5)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto id = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto r = static_cast<uint8_t>(lua_tointeger(state, 2));
   const auto g = static_cast<uint8_t>(lua_tointeger(state, 3));
   const auto b = static_cast<uint8_t>(lua_tointeger(state, 4));
   const auto a = static_cast<uint8_t>(lua_tointeger(state, 5));
   node->setSpriteColor(id, r, g, b, a);

   return 0;
}

/**
 * @brief queryAABB do an aabb query
 * @param state lua state
 *    param 1: aabb x1
 *    param 2: aabb y1
 *    param 3: aabb x2
 *    param 4: aabb y2
 *    return hit count
 * @return 1 if hit, 0 if no hit
 */
int32_t queryAABB(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x1 = static_cast<float>(lua_tointeger(state, 1) * MPP);
   const auto y1 = static_cast<float>(lua_tointeger(state, 2) * MPP);
   const auto x2 = static_cast<float>(lua_tointeger(state, 3) * MPP);
   const auto y2 = static_cast<float>(lua_tointeger(state, 4) * MPP);

   b2AABB aabb;
   b2Vec2 lower;
   b2Vec2 upper;
   lower.Set(x1, y1);
   upper.Set(x2, y2);
   aabb.lowerBound = lower;
   aabb.upperBound = upper;

   const auto hit_count = node->queryAABB(aabb);
   lua_pushinteger(state, hit_count);

   return 1;
}

/**
 * @brief queryRayCast do a raycast and see if we hit something
 * @param state lua state
 *    param 1 x1
 *    param 2 y1
 *    param 3 x2
 *    param 4 y2
 *    return number of objects hit
 * @return exit code
 */
int32_t queryRayCast(lua_State* state)
{
   const auto argc = lua_gettop(state);

   if (argc != 4)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x1 = static_cast<float>(lua_tointeger(state, 1) * MPP);
   const auto y1 = static_cast<float>(lua_tointeger(state, 2) * MPP);
   const auto x2 = static_cast<float>(lua_tointeger(state, 3) * MPP);
   const auto y2 = static_cast<float>(lua_tointeger(state, 4) * MPP);

   b2Vec2 p1;
   b2Vec2 p2;
   p1.Set(x1, y1);
   p2.Set(x2, y2);

   const auto hit_count = node->queryRaycast(p1, p2);
   lua_pushinteger(state, hit_count);
   return 1;
}

/**
 * @brief setDamageToPlayer set the damage of this lua node
 * @param state lua state
 *    param damage amount of damage (0..100)
 * @return error code
 */
int32_t setDamageToPlayer(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto damage_amount = static_cast<int32_t>(lua_tointeger(state, 1));
   node->setDamageToPlayer(damage_amount);

   return 0;
}

/**
 * @brief setZ set the z layer of this node
 * @param state lua state
 *    param 1: z layer
 * @return exit code
 */
int32_t setZIndex(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto z = static_cast<int32_t>(lua_tointeger(state, 1));
   node->_z_index = z;

   return 0;
}

/**
 * @brief makeDynamic make this object a dynamic box2d object
 * @param state lua state
 * @return exit code
 */
int32_t makeDynamic(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->makeDynamic();
   return 0;
}

/**
 * @brief makeStatic make this object a static box2d object
 * @param state lua state
 * @return exit code
 */
int32_t makeStatic(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->makeStatic();
   return 0;
}

/**
 * @brief setGravityScale set the gravity scale of this node
 * @param state lua state
 *    param 1: gravity scale (0..1)
 * @return error code
 */
int32_t setGravityScale(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto scale = static_cast<float>(lua_tonumber(state, 1));
   node->setGravityScale(scale);

   return 0;
}

/**
 * @brief setActive set this node active/inactive
 * @param state lua state
 *    param 1: active flag
 * @return error code
 */
int32_t setActive(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc == 1)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto active = static_cast<bool>(lua_toboolean(state, 1));
   node->setActive(active);

   return 0;
}

/**
 * @brief isPhsyicsPathClear check if a given path hits objects inside the tmx
 * @param state lua state
 *    param 1: x0
 *    param 2: y0
 *    param 3: x1
 *    param 4: y1
 *    return \c true on collision
 * @return error code
 */
int32_t isPhsyicsPathClear(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   // the lua scripts think in pixels; the physics grid has a resolution of 8x8 for each tile.
   // so that needs to be scaled first.
   const auto x0 = static_cast<int32_t>(lua_tonumber(state, 1) / PIXELS_PER_PHYSICS_TILE);
   const auto y0 = static_cast<int32_t>(lua_tonumber(state, 2) / PIXELS_PER_PHYSICS_TILE);
   const auto x1 = static_cast<int32_t>(lua_tonumber(state, 3) / PIXELS_PER_PHYSICS_TILE);
   const auto y1 = static_cast<int32_t>(lua_tonumber(state, 4) / PIXELS_PER_PHYSICS_TILE);

   // check map for collision
   const auto collides = Level::getCurrentLevel()->isPhysicsPathClear({x0, y0}, {x1, y1});
   lua_pushboolean(state, !collides);

   return 1;
}

/**
 * @brief getLinearVelocity reads the linear velocity of this object
 * @param state lua state
 *    return table
 *       1: velocity x
 *       2: velocity y
 * @return error code
 */
int32_t getLinearVelocity(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto velocity = node->getLinearVelocity();

   lua_createtable(state, 2, 0);

   auto table = lua_gettop(state);
   auto index = 1;

   lua_pushnumber(state, static_cast<double>(velocity.x));
   lua_rawseti(state, table, index++);
   lua_pushnumber(state, static_cast<double>(velocity.y));
   lua_rawseti(state, table, index++);

   return 1;
}

/**
 * @brief setLinearVelocity setter for linear velocity
 * @param state lua state
 *    param 1: velocity x
 *    param 2: velocity y
 * @return error code
 */
int32_t setLinearVelocity(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 2)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto vx = static_cast<float>(lua_tonumber(state, 1));
   const auto vy = static_cast<float>(lua_tonumber(state, 2));
   node->setLinearVelocity(b2Vec2{vx, vy});

   return 0;
}

/**
 * @brief damage the node sets some damage to the player
 * @param state lua state
 *    param 1: amount of damage from 0..100
 *    param 2: dx damage direction x
 *    param 3: dy damage direction y
 * @return error code
 */
int32_t damage(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto damage_amount = static_cast<int32_t>(lua_tonumber(state, 1));
   const auto dx = static_cast<float>(lua_tonumber(state, 2));
   const auto dy = static_cast<float>(lua_tonumber(state, 3));
   node->damagePlayer(damage_amount, dx, dy);

   Log::Info() << "damage: " << damage_amount << " dx: " << dx << " dy: " << dy;

   return 0;
}

/**
 * @brief damage the node damages the palyer if he's within a given radius
 * @param state lua state
 *    param 1: amount of damage from 0..100
 *    param 2: dx damage direction x
 *    param 3: dy damage direction y
 *    param 4: radius damage radius
 * @return error code
 */
int32_t damageRadius(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto damage_amount = static_cast<int32_t>(lua_tonumber(state, 1));
   const auto x = static_cast<float>(lua_tonumber(state, 2));
   const auto y = static_cast<float>(lua_tonumber(state, 3));
   const auto radius = static_cast<float>(lua_tonumber(state, 4));
   node->damagePlayerInRadius(damage_amount, x, y, radius);

   return 0;
}

/**
 * @brief setTransform set the object's transform
 * @param state lua state
 *    param 1: x translation
 *    param 2: y translation
 *    param 3: z rotation
 * @return error code
 */
int32_t setTransform(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x = static_cast<float>(lua_tonumber(state, 1));
   const auto y = static_cast<float>(lua_tonumber(state, 2));
   const auto angle = static_cast<float>(lua_tonumber(state, 3));
   b2Vec2 pos{x / PPM, y / PPM};
   node->setTransform(pos, angle);

   return 0;
}

/**
 * @brief addSprite add another (empty) sprite to this node
 * @param state lua state
 * @return error code
 */
int32_t addSprite(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->addSprite();

   return 0;
}

/**
 * @brief setSpriteOrigin set origin of a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: x position
 *    param 3: y position
 * @return error code
 */
int32_t setSpriteOrigin(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto id = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto x = static_cast<float>(lua_tonumber(state, 2));
   const auto y = static_cast<float>(lua_tonumber(state, 3));
   node->setSpriteOrigin(id, x, y);

   return 0;
}

/**
 * @brief setSpriteOffset sets the offset for a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: x position
 *    param 3: y position
 * @return error code
 */
int32_t setSpriteOffset(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto id = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto x = static_cast<float>(lua_tonumber(state, 2));
   const auto y = static_cast<float>(lua_tonumber(state, 3));
   node->setSpriteOffset(id, x, y);

   return 0;
}

/**
 * @brief boom make the game go booom
 * @param state lua state
 *    param 1: detonation center x
 *    param 2: detonation center y
 *    param 3: boom intensity
 * @return error code
 */
int32_t boom(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x = static_cast<float>(lua_tonumber(state, 1));
   const auto y = static_cast<float>(lua_tonumber(state, 2));
   const auto intensity = static_cast<float>(lua_tonumber(state, 3));
   node->boom(x, y, intensity);

   return 0;
}

/**
 * @brief play a detonation animation
 * @param state lua state
 *    param 1: detonation center x
 *    param 2: detonation center y
 * @return error code
 */
int32_t playDetonationAnimation(lua_State* state)
{
   const auto argc = lua_gettop(state);

   if (argc != 2)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto x = static_cast<float>(lua_tonumber(state, 1));
   const auto y = static_cast<float>(lua_tonumber(state, 2));
   node->playDetonationAnimation(x, y);
   return 0;
}

/**
 * @brief addShapeCircle add a circle shape to the node
 * @param state lua state
 *    param 1: circle radius
 *    param 2: circle x position
 *    param 3: circle y position
 * @return error code
 */
int32_t addShapeCircle(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto r = static_cast<float>(lua_tonumber(state, 1));
   const auto x = static_cast<float>(lua_tonumber(state, 2));
   const auto y = static_cast<float>(lua_tonumber(state, 3));
   node->addShapeCircle(r, x, y);

   return 0;
}

/**
 * @brief addShapeRect add a rectangular shape to the node
 * @param state lua state
 *    param 1: rect width
 *    param 2: rect height
 *    param 3: rect position x
 *    param 4: rect position y
 * @return error code
 */
int32_t addShapeRect(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 4)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto width = static_cast<float>(lua_tonumber(state, 1));
   const auto height = static_cast<float>(lua_tonumber(state, 2));
   const auto x = static_cast<float>(lua_tonumber(state, 3));
   const auto y = static_cast<float>(lua_tonumber(state, 4));
   node->addShapeRect(width, height, x, y);

   return 0;
}

/**
 * @brief addShapePoly add a polygonal shape to the node
 * @param state lua state
 *    param n x coordinate
 *    param n + 1 y coordinate
 * @return error code
 */
int32_t addShapePoly(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto argc = lua_gettop(state);
   if (argc >= 2 && (argc % 2 == 0))
   {
      const auto size = argc / 2;
      auto poly = new b2Vec2[static_cast<uint32_t>(size)];
      auto poly_index = 0;
      for (auto i = 0; i < argc; i += 2)
      {
         const auto x = static_cast<float>(lua_tonumber(state, i));
         const auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[poly_index].Set(x, y);
         poly_index++;
      }

      node->addShapePoly(poly, size);
   }

   return 0;
}

/**
 * @brief addWeapon add a weapon instance to the player
 * @param state lua state
 *    param 1: weapon type (enum)
 *    param 2: fire interval in ms
 *    param 3: damage for single hit (0..100)
 *    param 4: bullet radius
 *    param 4..n: polygon x and y parameters if not a radial bullet
 * @return error code
 */
int32_t addWeapon(lua_State* state)
{
   const auto argc = static_cast<size_t>(lua_gettop(state));
   if (argc < 3)
   {
      Log::Error() << "bad parameters for addWeapon";
      exit(1);
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   std::unique_ptr<b2Shape> shape;

   const auto weapon_type = static_cast<WeaponType>(lua_tointeger(state, 1));
   const auto fire_interval = static_cast<int>(lua_tointeger(state, 2));
   const auto damage_value = static_cast<int>(lua_tointeger(state, 3));

   // add weapon with projectile radius only
   if (argc == 4)
   {
      shape = std::make_unique<b2CircleShape>();

      const auto radius = static_cast<float>(lua_tonumber(state, 4));
      dynamic_cast<b2CircleShape*>(shape.get())->m_radius = radius;
   }

   // add weapon with polygon projectile shape
   if (argc >= 5 && ((argc - 5) % 2 == 0))
   {
      shape = std::make_unique<b2PolygonShape>();

      constexpr auto parameter_count = 2u;
      auto poly = new b2Vec2[(argc - parameter_count) / 2];
      auto poly_index = 0;

      for (auto i = parameter_count + 1; i < argc - parameter_count; i += 2u)
      {
         const auto x = static_cast<float>(lua_tonumber(state, i));
         const auto y = static_cast<float>(lua_tonumber(state, i + 1));
         poly[poly_index].Set(x, y);
         poly_index++;
      }

      dynamic_cast<b2PolygonShape*>(shape.get())->Set(poly, poly_index);
   }

   auto weapon = WeaponFactory::create(node->_body, weapon_type, std::move(shape), fire_interval, damage_value);
   node->addWeapon(std::move(weapon));

   return 0;
}

/**
 * @brief useGun fire a weapon
 * @param state lua state
 *    param 1: index of the weapon
 *    param 2: x position where the shot comes from
 *    param 3: y position where the shot comes from
 *    param 4: x direction
 *    param 5: y direction
 * @return error code
 */
int32_t useGun(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 5)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto index = static_cast<size_t>(lua_tointeger(state, 1));
   const auto pos_x = static_cast<float>(lua_tonumber(state, 2)) * MPP;
   const auto pos_y = static_cast<float>(lua_tonumber(state, 3)) * MPP;
   const auto dir_x = static_cast<float>(lua_tonumber(state, 4));
   const auto dir_y = static_cast<float>(lua_tonumber(state, 5));
   node->useGun(index, {pos_x, pos_y}, {dir_x, dir_y});

   return 0;
}

/**
 * @brief updateProjectileTexture change the texture of a projectile
 * @param state lua state
 *    param 1: index of the weapon
 *    param 2: path of the texture
 *    param 3: x position of the texture rect
 *    param 4: y position of the texture rect
 *    param 5: width of the texture rect
 *    param 6: height of the texture rect
 * @return error code
 */
int32_t updateProjectileTexture(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto argc = lua_gettop(state);
   const auto valid = (argc >= 2);

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
      const auto x1 = static_cast<int32_t>(lua_tointeger(state, 3));
      const auto y1 = static_cast<int32_t>(lua_tointeger(state, 4));
      const auto width = static_cast<int32_t>(lua_tointeger(state, 5));
      const auto height = static_cast<int32_t>(lua_tointeger(state, 6));

      rect.left = x1;
      rect.top = y1;
      rect.width = width;
      rect.height = height;
   }

   if (valid)
   {
      const auto& texture = TexturePool::getInstance().get(path);
      dynamic_cast<Gun&>(*node->_weapons[index]).setProjectileAnimation(texture, rect);
   }

   return 0;
}

/**
 * @brief updateProjectileAnimation set projectile animation for a given weapon
 * @param state lua state
 *    param 1: weapon index
 *    param 2: texture path
 *    param 3: width of one frame
 *    param 4: height of one frame
 *    param 5: x origin of the frame
 *    param 6: y origin of the frame
 *    param 7: time for each frame in seconds
 *    param 8: frame count
 *    param 9: frames per row
 *    param 10: start frame
 * @return error code
 */
int32_t updateProjectileAnimation(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 10)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto weapon_index = static_cast<uint32_t>(lua_tointeger(state, 1));
   const std::filesystem::path path = lua_tostring(state, 2);
   const auto frame_width = static_cast<uint32_t>(lua_tointeger(state, 3));
   const auto frame_height = static_cast<uint32_t>(lua_tointeger(state, 4));
   const auto frame_origin_x = static_cast<float>(lua_tointeger(state, 5));
   const auto frame_origin_y = static_cast<float>(lua_tointeger(state, 6));
   const auto time_per_frame_s = static_cast<float>(lua_tonumber(state, 7));
   const auto frame_count = static_cast<uint32_t>(lua_tointeger(state, 8));
   const auto frames_per_row = static_cast<uint32_t>(lua_tointeger(state, 9));
   const auto start_frame = static_cast<uint32_t>(lua_tointeger(state, 10));
   const auto texture = TexturePool::getInstance().get(path);
   const sf::Vector2f frame_origin{frame_origin_x, frame_origin_y};

   // assume identical frame times for now
   std::vector<sf::Time> frame_times_s;
   for (auto i = 0u; i < frame_count; i++)
   {
      frame_times_s.push_back(sf::seconds(time_per_frame_s));
   }

   AnimationFrameData frame_data(texture, frame_origin, frame_width, frame_height, frame_count, frames_per_row, frame_times_s, start_frame);

   dynamic_cast<Gun&>(*node->_weapons[weapon_index]).setProjectileAnimation(frame_data);

   return 0;
}

/**
 * @brief timer start a timer
 * @param state lua state
 *    param 1: delay of the timer
 *    param 2: id of the timer in milliseconds
 * @return error code
 */
int32_t timer(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 2)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto delay = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto timer_id = static_cast<int32_t>(lua_tointeger(state, 2));

   Timer::add(
      std::chrono::milliseconds(delay),
      [node, timer_id]() { node->luaTimeout(timer_id); },
      Timer::Type::Singleshot,
      Timer::Scope::UpdateIngame,
      nullptr,
      node
   );

   return 0;
}

/**
 * @brief addSample add a sample to be played later
 * @param state lua state
 *    param 1: name of the sample
 * @return error code
 */
int32_t addSample(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto sample = lua_tostring(state, 1);
   Audio::getInstance().addSample(sample);

   return 0;
}

/**
 * @brief playSample play a sample
 * @param state lua state
 *    param 1: name of the sample to play
 *    param 2: volume (0..1)
 * @return
 */
int32_t playSample(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 2)
   {
      return 0;
   }

   const auto sample = lua_tostring(state, 1);
   const auto volume = static_cast<float>(lua_tonumber(state, 2));
   Audio::getInstance().playSample(sample, volume);

   return 0;
}

/**
 * @brief debug output a debug message to stdout
 * @param state lua state
 *    param 1: debug message
 * @return error code
 */
int32_t debug(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto message = lua_tostring(state, 1);
   puts(message);

   return 0;
}

/**
 * @brief registerHitAnimation register a hit animation for a given weapon
 * @param state lua state
 *    param 1: weapon index
 *    param 2: texture path
 *    param 3: width of one frame
 *    param 4: height of one frame
 *    param 5: frame count
 *    param 6: frames per row
 *    param 7: start frame
 * @return error code
 */
int32_t registerHitAnimation(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 8)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto weapon_index = static_cast<uint32_t>(lua_tointeger(state, 1));
   const std::filesystem::path path = lua_tostring(state, 2);
   const auto frame_width = static_cast<uint32_t>(lua_tointeger(state, 3));
   const auto frame_height = static_cast<uint32_t>(lua_tointeger(state, 4));
   const auto time_per_frame_s = static_cast<float>(lua_tonumber(state, 5));
   const auto frame_count = static_cast<uint32_t>(lua_tointeger(state, 6));
   const auto frames_per_row = static_cast<uint32_t>(lua_tointeger(state, 7));
   const auto start_frame = static_cast<uint32_t>(lua_tointeger(state, 8));

   ProjectileHitAnimation::addReferenceAnimation(
      path,
      frame_width,
      frame_height,
      std::chrono::duration<float, std::chrono::seconds::period>{time_per_frame_s},
      frame_count,
      frames_per_row,
      start_frame
   );

   dynamic_cast<Gun&>(*node->_weapons[weapon_index]).setProjectileIdentifier(path.string());

   return 0;
}

/**
 * @brief updateKeysPressed fire keypressed events to the node instance
 * @param state lua state
 *    param 1: keypressed bitmask
 * @return error code
 */
int32_t updateKeysPressed(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto keys_pressed = static_cast<int32_t>(lua_tointeger(state, 1));
   node->_keys_pressed = keys_pressed;

   return 0;
}

/**
 * @brief die let the node die
 * @param state lua state
 * @return error code
 */
int32_t die(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->luaDie();
   return 0;
}

[[noreturn]] void error(lua_State* state, const char* /*scope*/ = nullptr)
{
   // the error message is on top of the stack.
   // fetch it, print32_t it and then pop it off the stack.
   std::stringstream os;
   os << lua_tostring(state, -1);

   Log::Error() << os.str();

   lua_pop(state, 1);

   exit(1);
}

}  // namespace

//-----------------------------------------------------------------------------

void LuaNode::setupTexture()
{
   std::string spriteName = std::get<std::string>(_properties["sprite"]);

   _texture = TexturePool::getInstance().get(spriteName);

   for (auto& sprite : _sprites)
   {
      sprite.setTexture(*_texture);
   }
}

LuaNode::LuaNode(GameNode* parent, const std::string& filename) : GameNode(parent), _script_name(filename)
{
   setClassName(typeid(LuaNode).name());

   // create instances
   _body_def = new b2BodyDef();
   _body = Level::getCurrentLevel()->getWorld()->CreateBody(_body_def);
}

LuaNode::~LuaNode()
{
   stopScript();
}

void LuaNode::deserializeEnemyDescription()
{
   // set up patrol path
   if (!_enemy_description._path.empty())
   {
      std::vector<sf::Vector2f> patrol_path;

      for (auto i = 0u; i < _enemy_description._path.size(); i += 2)
      {
         auto pos =
            sf::Vector2f(static_cast<float_t>(_enemy_description._path.at(i)), static_cast<float_t>(_enemy_description._path.at(i + 1)));

         // by default the path is given is tiles.
         // if we override it, we're setting pixel positions which are already transformed
         if (_enemy_description._position_in_tiles)
         {
            pos.x *= PIXELS_PER_TILE;
            pos.y *= PIXELS_PER_TILE;
            pos.x += PIXELS_PER_TILE / 2;
            pos.y += PIXELS_PER_TILE / 2;
         }

         patrol_path.push_back(pos);
      }

      _movement_path_px = patrol_path;
   }

   // set up start position
   if (!_enemy_description._start_position.empty())
   {
      _start_position_px = sf::Vector2f(
         static_cast<float_t>(_enemy_description._start_position.at(0)), static_cast<float_t>(_enemy_description._start_position.at(1))
      );

      if (_enemy_description._position_in_tiles)
      {
         _start_position_px.x *= PIXELS_PER_TILE;
         _start_position_px.y *= PIXELS_PER_TILE;
         _start_position_px.x += PIXELS_PER_TILE / 2;
         _start_position_px.y += PIXELS_PER_TILE / 2;
      }

      _position_px = _start_position_px;
   }

   setObjectId(_enemy_description._id);
}

void LuaNode::initialize()
{
   deserializeEnemyDescription();
   setupLua();
   setupBody();
}

void LuaNode::setupLua()
{
   _lua_state = luaL_newstate();

   // register callbacks
   lua_register(_lua_state, "addHitbox", ::addHitbox);
   lua_register(_lua_state, "addSample", ::addSample);
   lua_register(_lua_state, "addShapeCircle", ::addShapeCircle);
   lua_register(_lua_state, "addShapeRect", ::addShapeRect);
   lua_register(_lua_state, "addSprite", ::addSprite);
   lua_register(_lua_state, "addWeapon", ::addWeapon);
   lua_register(_lua_state, "boom", ::boom);
   lua_register(_lua_state, "damage", ::damage);
   lua_register(_lua_state, "damageRadius", ::damageRadius);
   lua_register(_lua_state, "debug", ::debug);
   lua_register(_lua_state, "die", ::die);
   lua_register(_lua_state, "getLinearVelocity", ::getLinearVelocity);
   lua_register(_lua_state, "isPhsyicsPathClear", ::isPhsyicsPathClear);
   lua_register(_lua_state, "makeDynamic", ::makeDynamic);
   lua_register(_lua_state, "makeStatic", ::makeStatic);
   lua_register(_lua_state, "playDetonationAnimation", ::playDetonationAnimation);
   lua_register(_lua_state, "playSample", ::playSample);
   lua_register(_lua_state, "queryAABB", ::queryAABB);
   lua_register(_lua_state, "queryRayCast", ::queryRayCast);
   lua_register(_lua_state, "registerHitAnimation", ::registerHitAnimation);
   lua_register(_lua_state, "setActive", ::setActive);
   lua_register(_lua_state, "setDamage", ::setDamageToPlayer);
   lua_register(_lua_state, "setGravityScale", ::setGravityScale);
   lua_register(_lua_state, "setLinearVelocity", ::setLinearVelocity);
   lua_register(_lua_state, "setSpriteOffset", ::setSpriteOffset);
   lua_register(_lua_state, "setSpriteOrigin", ::setSpriteOrigin);
   lua_register(_lua_state, "setSpriteColor", ::setSpriteColor);
   lua_register(_lua_state, "setTransform", ::setTransform);
   lua_register(_lua_state, "setZ", ::setZIndex);
   lua_register(_lua_state, "timer", ::timer);
   lua_register(_lua_state, "updateKeysPressed", ::updateKeysPressed);
   lua_register(_lua_state, "updateProjectileAnimation", ::updateProjectileAnimation);
   lua_register(_lua_state, "updateProjectileTexture", ::updateProjectileTexture);
   lua_register(_lua_state, "updateProperties", ::updateProperties);
   lua_register(_lua_state, "updateSpriteRect", ::updateSpriteRect);
   lua_register(_lua_state, "useGun", ::useGun);

   // make standard libraries available in the Lua object
   luaL_openlibs(_lua_state);

   // load program
   auto result = luaL_loadfile(_lua_state, _script_name.c_str());
   if (result == LUA_OK)
   {
      // execute program
      result = lua_pcall(_lua_state, 0, LUA_MULTRET, 0);

      if (result != LUA_OK)
      {
         error(_lua_state);
      }
      else
      {
         luaSetStartPosition();
         luaMovedTo();
         luaInitialize();
         luaRetrieveProperties();
         luaSendPatrolPath();
      }
   }
   else
   {
      error(_lua_state);
   }

   // register properties
   for (auto& prop : _enemy_description._properties)
   {
      luaWriteProperty(prop._name, prop._value);
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

/**
 * @brief LuaNode::luaInitialize called to call the initialize function inside the lua script
 * callback name: initialize
 */
void LuaNode::luaInitialize()
{
   lua_getglobal(_lua_state, FUNCTION_INITIALIZE);
   const auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_INITIALIZE);
   }
}

/**
 * @brief LuaNode::luaUpdate update the lua node
 * @param dt delta time, passed to luanode in seconds
 * callback name: update
 */
void LuaNode::luaUpdate(const sf::Time& dt)
{
   lua_getglobal(_lua_state, FUNCTION_UPDATE);
   lua_pushnumber(_lua_state, dt.asSeconds());

   const auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_UPDATE);
   }
}

/**
 * @brief LuaNode::luaWriteProperty write a property of the luanode
 * @param key property key
 * @param value property value
 * callback name: writeProperty
 */
void LuaNode::luaWriteProperty(const std::string& key, const std::string& value)
{
   lua_getglobal(_lua_state, FUNCTION_WRITE_PROPERTY);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, key.c_str());
      lua_pushstring(_lua_state, value.c_str());

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_WRITE_PROPERTY);
      }
   }
}

/**
 * @brief LuaNode::luaHit luanode got hit by something
 * @param damage amount of damage from 0..100 while 100 is fatal
 * callback name: hit
 */
void LuaNode::luaHit(int32_t damage)
{
   // Log::Info() << "thing was hit: " << damage;
   _hit_time = std::chrono::high_resolution_clock::now();
   _damage_from_player = damage;

   lua_getglobal(_lua_state, FUNCTION_HIT);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushinteger(_lua_state, damage);

      const auto result = lua_pcall(_lua_state, 1, 0, 0);
      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_HIT);
      }
   }
}

/**
 * @brief LuaNode::luaCollisionWithPlayer indicate collision with player
 * callback name: collisionWithPlayer
 */
void LuaNode::luaCollisionWithPlayer()
{
   lua_getglobal(_lua_state, FUNCTION_COLLISION_WITH_PLAYER);
   if (lua_isfunction(_lua_state, -1))
   {
      const auto result = lua_pcall(_lua_state, 0, 0, 0);
      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_COLLISION_WITH_PLAYER);
      }
   }
}

/**
 * @brief LuaNode::luaSendPatrolPath sends the patrol path coordinates to the lua script
 * callback name: setPath
 */
void LuaNode::luaSendPatrolPath()
{
   if (_movement_path_px.size() == 0)
   {
      return;
   }

   lua_getglobal(_lua_state, FUNCTION_SET_PATH);

   lua_pushstring(_lua_state, "path");
   luaSendPath(_movement_path_px);

   // vec.size + 1 args, 0 result
   const auto result = lua_pcall(_lua_state, 2, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_SET_PATH);
   }
}

/**
 * @brief LuaNode::luaDie lua script is told to die
 */
void LuaNode::luaDie()
{
   Level::getCurrentLevel()->getWorld()->DestroyBody(_body);

   // resetting the body will get it removed from the luainterface class
   _body = nullptr;
}

/**
 * @brief LuaNode::luaMovedTo tell lua script where the engine moved it to
 * callback name: movedTo
 * lua param x: x position (double)
 * lua param y: y position (double)
 */
void LuaNode::luaMovedTo()
{
   const auto x = _position_px.x;
   const auto y = _position_px.y;

   lua_getglobal(_lua_state, FUNCTION_MOVED_TO);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, static_cast<double>(x));
      lua_pushnumber(_lua_state, static_cast<double>(y));

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_MOVED_TO);
      }
   }
}

/**
 * @brief LuaNode::luaSetStartPosition
 * callback name: setStartPosition
 * lua param x: x position of start position (double)
 * lua param y: y position of start position (double)
 */
void LuaNode::luaSetStartPosition()
{
   const auto x = _start_position_px.x;
   const auto y = _start_position_px.y;

   lua_getglobal(_lua_state, FUNCTION_SET_START_POSITION);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, static_cast<double>(x));
      lua_pushnumber(_lua_state, static_cast<double>(y));

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_SET_START_POSITION);
      }
   }
}

/**
 * @brief LuaNode::luaPlayerMovedTo engine moved player to a certain position
 * callback name: playerMovedTo
 * lua param x: x position of player position (double)
 * lua param y: y position of player position (double)
 */
void LuaNode::luaPlayerMovedTo()
{
   const auto pos = Player::getCurrent()->getPixelPositionFloat();

   lua_getglobal(_lua_state, FUNCTION_PLAYER_MOVED_TO);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, pos.x);
      lua_pushnumber(_lua_state, pos.y);

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         error(_lua_state, FUNCTION_PLAYER_MOVED_TO);
      }
   }
}

/**
 * @brief LuaNode::luaRetrieveProperties instruct lua node to retrieve properties now
 * callback name: retrieveProperties
 */
void LuaNode::luaRetrieveProperties()
{
   lua_getglobal(_lua_state, FUNCTION_RETRIEVE_PROPERTIES);

   // 0 args, 0 result
   const auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_RETRIEVE_PROPERTIES);
   }
}

/**
 * @brief LuaNode::luaTimeout timeout timer fired
 * @param timerId timer id of timeout timer
 * callback name: timeout
 * lua param timerId: id of timeout timer
 */
void LuaNode::luaTimeout(int32_t timerId)
{
   lua_getglobal(_lua_state, FUNCTION_TIMEOUT);
   lua_pushinteger(_lua_state, timerId);

   const auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      error(_lua_state, FUNCTION_TIMEOUT);
   }
}

/**
 * @brief LuaNode::luaSendPath inject a path into the current lua state
 * @param vec vector of 2d vectors
 */
void LuaNode::luaSendPath(const std::vector<sf::Vector2f>& vec)
{
   lua_newtable(_lua_state);

   int32_t i = 0;
   for (const auto& v : vec)
   {
      lua_pushnumber(_lua_state, v.x);  // push x
      lua_rawseti(_lua_state, -2, ++i);
      lua_pushnumber(_lua_state, v.y);  // push y
      lua_rawseti(_lua_state, -2, ++i);
   }
}

void LuaNode::damagePlayerInRadius(int32_t damage, float x, float y, float radius)
{
   sf::Vector2f node_position{x, y};
   const auto player_position = Player::getCurrent()->getPixelPositionFloat();

   auto dist = (player_position - node_position);
   auto len = SfmlMath::length(dist);

   if (len <= radius)
   {
      // does it really make sense to normalize this vector?
      Player::getCurrent()->damage(damage, SfmlMath::normalize(-dist));
   }
}

void LuaNode::damagePlayer(int32_t damage, float forceX, float forceY)
{
   Player::getCurrent()->damage(damage, sf::Vector2f(forceX, forceY));
}

b2Vec2 LuaNode::getLinearVelocity() const
{
   b2Vec2 velocity;
   velocity.SetZero();

   if (_body)
   {
      velocity = _body->GetLinearVelocity();
   }

   return velocity;
}

void LuaNode::setLinearVelocity(const b2Vec2& vel)
{
   if (_body)
   {
      _body->SetLinearVelocity(vel);
   }
}

void LuaNode::boom(float x, float y, float intensity)
{
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, intensity);
}

void LuaNode::playDetonationAnimation(float x, float y)
{
   auto detonation = DetonationAnimation::makeHugeExplosion(sf::Vector2f{x, y});
   AnimationPlayer::getInstance().add(detonation.getAnimations());
}

void LuaNode::setGravityScale(float scale)
{
   _body->SetGravityScale(scale);
}

void LuaNode::setTransform(const b2Vec2& position, float32 angle)
{
   _body->SetTransform(position, angle);
}

void LuaNode::addSprite()
{
   _sprites.push_back({});
   _sprite_offsets_px.push_back({});
}

void LuaNode::setSpriteOrigin(int32_t id, float x, float y)
{
   _sprites[id].setOrigin(x, y);
}

void LuaNode::setSpriteOffset(int32_t id, float x, float y)
{
   _sprite_offsets_px[id].x = x;
   _sprite_offsets_px[id].y = y;
}

void LuaNode::setActive(bool active)
{
   _body->SetActive(active);
}

void LuaNode::setDamageToPlayer(int32_t damage)
{
   for (auto fixture = _body->GetFixtureList(); fixture; fixture = fixture->GetNext())
   {
      auto user_data = fixture->GetUserData();

      if (!user_data)
      {
         continue;
      }

      auto fixture_node = static_cast<FixtureNode*>(fixture->GetUserData());
      fixture_node->setProperty("damage", damage);
   }
}

void LuaNode::makeDynamic()
{
   _body->SetType(b2_dynamicBody);
}

void LuaNode::makeStatic()
{
   _body->SetType(b2_staticBody);
}

class LuaQueryCallback : public b2QueryCallback
{
public:
   std::vector<b2Body*> _bodies;

   bool ReportFixture(b2Fixture* fixture)
   {
      _bodies.push_back(fixture->GetBody());

      // to keep going to find all fixtures in the query area
      return true;
   }
};

int32_t LuaNode::queryAABB(const b2AABB& aabb)
{
   LuaQueryCallback query_callback;
   Level::getCurrentLevel()->getWorld()->QueryAABB(&query_callback, aabb);

   // Log::Info() << queryCallback.mBodies.size();
   return static_cast<int32_t>(query_callback._bodies.size());
}

class LuaRaycastCallback : public b2RayCastCallback
{
public:
   std::vector<b2Body*> _bodies;

   float ReportFixture(
      b2Fixture* fixture,
      const b2Vec2& /*point*/,
      const b2Vec2& /*normal*/,
      float32 /*fraction*/
   )
   {
      _bodies.push_back(fixture->GetBody());
      return 0.0f;
   }
};

int32_t LuaNode::queryRaycast(const b2Vec2& point1, const b2Vec2& point2)
{
   LuaRaycastCallback query_callback;
   Level::getCurrentLevel()->getWorld()->RayCast(&query_callback, point1, point2);

   // Log::Info() << queryCallback.mBodies.size();
   return static_cast<int32_t>(query_callback._bodies.size());
}

bool LuaNode::getPropertyBool(const std::string& key)
{
   auto value = false;
   auto it = _properties.find(key);
   if (it != _properties.end())
      value = std::get<bool>(it->second);
   return value;
}

double LuaNode::getPropertyDouble(const std::string& key)
{
   auto value = 0.0;
   auto it = _properties.find(key);
   if (it != _properties.end())
      value = std::get<double>(it->second);
   return value;
}

int64_t LuaNode::getPropertyInt64(const std::string& key)
{
   auto value = 0LL;
   auto it = _properties.find(key);
   if (it != _properties.end())
      value = std::get<int64_t>(it->second);
   return value;
}

void LuaNode::setupBody()
{
   auto static_body = getPropertyBool("staticBody");
   auto damage = static_cast<int32_t>(getPropertyInt64("damage"));
   auto sensor = static_cast<bool>(getPropertyBool("sensor"));

   _body->SetTransform(b2Vec2{_start_position_px.x * MPP, _start_position_px.y * MPP}, 0.0f);
   _body->SetFixedRotation(true);
   _body->SetType(static_body ? b2_staticBody : b2_dynamicBody);

   for (auto shape : _shapes_m)
   {
      b2FixtureDef fd;
      fd.density = 1.f;
      fd.friction = 0.0f;
      fd.restitution = 0.0f;
      fd.shape = shape;

      // apply default filter
      // http://www.iforce2d.net/b2dtut/collision-filtering
      fd.filter.groupIndex = group_index;
      fd.filter.maskBits = mask_bits_standing;
      fd.filter.categoryBits = category_bits;

      auto fixture = _body->CreateFixture(&fd);
      auto fixture_node = new FixtureNode(this);
      fixture_node->setType(ObjectTypeEnemy);
      fixture_node->setProperty("damage", damage);
      fixture_node->setCollisionCallback([this]() { luaCollisionWithPlayer(); });
      fixture->SetUserData(static_cast<void*>(fixture_node));

      if (sensor)
      {
         fixture->SetSensor(true);
      }
   }

   // mBody->Dump();
}

void LuaNode::addShapeCircle(float radius, float x, float y)
{
   auto shape = new b2CircleShape();
   shape->m_p.Set(x, y);
   shape->m_radius = radius;
   _shapes_m.push_back(shape);
}

void LuaNode::addShapeRect(float width, float height, float x, float y)
{
   auto shape = new b2PolygonShape();
   shape->SetAsBox(width, height, b2Vec2(x, y), 0.0f);
   _shapes_m.push_back(shape);
}

void LuaNode::addShapePoly(const b2Vec2* points, int32_t size)
{
   auto shape = new b2PolygonShape();
   shape->Set(points, size);
   _shapes_m.push_back(shape);
}

void LuaNode::addWeapon(std::unique_ptr<Weapon> weapon)
{
   weapon->initialize();
   _weapons.push_back(std::move(weapon));
}

void LuaNode::useGun(size_t index, b2Vec2 from, b2Vec2 to)
{
   dynamic_cast<Gun&>(*_weapons[index]).useInIntervals(Level::getCurrentLevel()->getWorld(), from, to);
}

void LuaNode::stopScript()
{
   if (_lua_state)
   {
      lua_close(_lua_state);
      _lua_state = nullptr;
   }
}

int32_t LuaNode::getDamageFromPlayer() const
{
   return _damage_from_player;
}

const LuaNode::HighResTimePoint& LuaNode::getHitTime() const
{
   return _hit_time;
}

void LuaNode::updateVelocity()
{
   if (!_body)
   {
      return;
   }

   auto velocity_max = 0.0;
   auto acceleration = 0.0;

   auto velocity_it = _properties.find("velocity_walk_max");
   if (velocity_it != _properties.end())
   {
      velocity_max = *std::get_if<double>(&(velocity_it->second));
   }

   auto acceleration_it = _properties.find("acceleration_ground");
   if (acceleration_it != _properties.end())
   {
      acceleration = *std::get_if<double>(&(acceleration_it->second));
   }

   auto desired_velocity = 0.0f;
   auto velocity = _body->GetLinearVelocity();

   if (_keys_pressed & KeyPressedLeft)
   {
      desired_velocity = static_cast<float>(b2Max(velocity.x - acceleration, -velocity_max));
   }

   if (_keys_pressed & KeyPressedRight)
   {
      desired_velocity = static_cast<float>(b2Min(velocity.x + acceleration, velocity_max));
   }

   // calc impulse, disregard time factor
   auto velocity_change = desired_velocity - velocity.x;
   auto impulse = _body->GetMass() * velocity_change;

   _body->ApplyLinearImpulse(b2Vec2(impulse, 0.0f), _body->GetWorldCenter(), true);
}

void LuaNode::updateWeapons(const sf::Time& dt)
{
   for (auto& w : _weapons)
   {
      w->update(dt);
   }
}

void LuaNode::updatePosition()
{
   if (!_body)
   {
      return;
   }

   auto x_px = _body->GetPosition().x * PPM;
   auto y_px = _body->GetPosition().y * PPM;

   _position_px.x = x_px;
   _position_px.y = y_px;

   // update hitbox positions
   for (auto& hitbox : _hitboxes)
   {
      hitbox._rect_px.left = x_px + hitbox._offset_px.x;
      hitbox._rect_px.top = y_px + hitbox._offset_px.y;
   }
}

void LuaNode::updateSpriteRect(int32_t id, int32_t x_px, int32_t y_px, int32_t w_px, int32_t h_px)
{
   if (!_sprites[id].getTexture() && _texture)
   {
      _sprites[id].setTexture(*_texture);
   }

   _sprites[id].setTextureRect(sf::IntRect(x_px, y_px, w_px, h_px));
}

void LuaNode::setSpriteColor(int32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
   _sprites[id].setColor({r, g, b, a});
}

void LuaNode::addHitbox(int32_t left_px, int32_t top_px, int32_t width_px, int32_t height_px)
{
   sf::FloatRect rect{0.0f, 0.0f, static_cast<float>(width_px), static_cast<float>(height_px)};
   sf::Vector2f offset{static_cast<float>(left_px), static_cast<float>(top_px)};
   Hitbox box{rect, offset};
   _hitboxes.push_back(box);
}

void LuaNode::draw(sf::RenderTarget& target)
{
   // draw sprite on top of projectiles
   for (auto& w : _weapons)
   {
      w->draw(target);
   }

   for (auto i = 0u; i < _sprites.size(); i++)
   {
      auto& sprite = _sprites[i];
      const auto& offset = _sprite_offsets_px[i];

      const auto center = sf::Vector2f(sprite.getTextureRect().width / 2.0f, sprite.getTextureRect().height / 2.0f);

      sprite.setPosition(_position_px - center + offset);

      target.draw(sprite);
   }
}
