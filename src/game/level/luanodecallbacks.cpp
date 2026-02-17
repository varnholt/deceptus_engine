// luanodecallbacks
#include "luanodecallbacks.h"

#include <cstdlib>
#include <lua.hpp>
#include <sstream>

// box2d
#include "box2d/box2d.h"

#include "framework/tools/log.h"
#include "game/animation/detonationanimation.h"
#include "game/constants.h"
#include "game/level/luainterface.h"

#define OBJINSTANCE LuaInterface::instance().getObject(state)

namespace LuaNodeCallbacks
{

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
      }
      if (lua_isnumber(state, -1))
      {
         if (lua_isinteger(state, -1))  // int64
         {
            OBJINSTANCE->_properties[key] = static_cast<int64_t>(lua_tointeger(state, -1));
         }
         else  // double
         {
            OBJINSTANCE->_properties[key] = lua_tonumber(state, -1);
         }
      }
      else if (lua_isstring(state, -1))  // string
      {
         OBJINSTANCE->_properties[key] = std::string(lua_tostring(state, -1));
      }

      lua_pop(state, 1);
   }

   OBJINSTANCE->synchronizeProperties();

   return 0;
}

/**
 * @brief addHitBox add a hitbox to the enemy
 * @param state lua state
 *    param 1: x position relative to where the object has been placed
 *    param 2: y position relative to where the object has been placed
 *    param 3: hitbox width
 *    param 4: hitbox height
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

int32_t addDebugRect(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc != 0)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->addDebugRect();

   return 0;
}

/**
 * @brief updateDebugRect add a debug rect to the enemy
 * @param state lua state
 *    param 1: index of the debug rect
 *    param 2: rect x position
 *    param 3: rect y position
 *    param 4: rect width
 *    param 5: rect height
 * @return error code
 */
int32_t updateDebugRect(lua_State* state)
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

   const auto index = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto x_px = static_cast<float>(lua_tonumber(state, 2));
   const auto y_px = static_cast<float>(lua_tonumber(state, 3));
   const auto w_px = static_cast<float>(lua_tonumber(state, 4));
   const auto h_px = static_cast<float>(lua_tonumber(state, 5));
   node->updateDebugRect(index, x_px, y_px, w_px, h_px);

   return 0;
}

/**
 * @brief addAudioRange add an audio range to the enemy
 * @param state lua state
 *    param 1: far distance in px
 *    param 2: far volume from 0..1
 *    param 3: near distance in px
 *    param 4: near volume from 0..1
 * @return error code
 */
int32_t addAudioRange(lua_State* state)
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

   const auto far_distance = static_cast<float>(lua_tonumber(state, 1));
   const auto far_volume = static_cast<float>(lua_tonumber(state, 2));
   const auto near_distance = static_cast<float>(lua_tonumber(state, 3));
   const auto near_volume = static_cast<float>(lua_tonumber(state, 4));
   node->addAudioRange(far_distance, far_volume, near_distance, near_volume);

   return 0;
}

/**
 * @brief setAudioUpdateBehavior change audio update behavior
 * @param state lua state
 *    param 1: update behavior
 * @return error code
 */
int32_t setAudioUpdateBehavior(lua_State* state)
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

   const auto audio_update_behavior = static_cast<AudioUpdateBehavior>(lua_tointeger(state, 1));
   node->setAudioUpdateBehavior(audio_update_behavior);

   return 0;
}

/**
 * @brief setReferenceVolume the volume to use when no audio range is used
 * @param state lua state
 *    param 1: update behavior
 * @return error code
 */
int32_t setReferenceVolume(lua_State* state)
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

   const auto reference_volume = static_cast<float>(lua_tonumber(state, 1));
   node->setReferenceVolume(reference_volume);

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
 * @brief setSpriteScale set scale of a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: x scale factor
 *    param 3: y scale factor
 * @return error code
 */
int32_t setSpriteScale(lua_State* state)
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
   const auto x_scale = static_cast<float>(lua_tonumber(state, 2));
   const auto y_scale = static_cast<float>(lua_tonumber(state, 3));
   node->setSpriteScale(id, x_scale, y_scale);

   return 0;
}

/**
 * @brief intersectsWithPlayer check if a rectangle intersects with the player's bounding rect
 * @param state lua state
 *    param 1: rect x
 *    param 2: rect y
 *    param 3: rect width
 *    param 4: rect height
 *    return hit count
 * @return 1 if there is an intersection, 0 if no intersection
 */
int32_t intersectsWithPlayer(lua_State* state)
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

   const auto x = static_cast<float>(lua_tonumber(state, 1));
   const auto y = static_cast<float>(lua_tonumber(state, 2));
   const auto width = static_cast<float>(lua_tonumber(state, 3));
   const auto height = static_cast<float>(lua_tonumber(state, 4));

   lua_pushboolean(state, node->intersectsPlayer(x, y, width, height));

   return 1;
}

/**
 * @brief isPlayerDead check if the player is dead
 * @param state lua state
 * @return 1 if player is dead, 0 if alive
 */
int32_t isPlayerDead(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      lua_pushboolean(state, false);
      return 1;
   }

   lua_pushboolean(state, node->checkPlayerDead());
   return 1;
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
   node->setZ(z);

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
   if (argc != 1)
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

   auto node = OBJINSTANCE;
   if (!node)
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
   lua_pushboolean(state, node->isPhysicsPathClear(x0, y0, x1, y1));

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
 * @brief getGravity reads the world's gravity
 * @param state lua state
 *    return table
 *       1: velocity x
 *       2: velocity y
 * @return error code
 */
int32_t getGravity(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   lua_pushnumber(state, node->getWorldGravity());

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
 * @brief applyLinearImpulse apply an impulse on the object
 * @param state lua state
 *    param 1: impulse x
 *    param 2: impulse y
 * @return error code
 */
int32_t applyLinearImpulse(lua_State* state)
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
   node->applyLinearImpulse(b2Vec2{vx, vy});

   return 0;
}

/**
 * @brief applyForce apply a force on the object
 * @param state lua state
 *    param 1: force x
 *    param 2: force y
 * @return error code
 */
int32_t applyForce(lua_State* state)
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

   node->applyForce(b2Vec2{vx, vy});

   return 0;
}

// todo: document
/**
 * @brief setVisible
 * @param state lua state
 *    param 1: visible flag
 * @return error code
 */
int32_t setVisible(lua_State* state)
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

   const auto visible = lua_toboolean(state, 1);
   node->setVisible(visible);

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
 * @brief setSpriteVisible sets the visibility for a given sprite
 * @param state lua state
 *    param 1: sprite id
 *    param 2: visibility flag (boolean)
 * @return error code
 */
int32_t setSpriteVisible(lua_State* state)
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

   const auto id = static_cast<int32_t>(lua_tointeger(state, 1));
   const auto visible = static_cast<bool>(lua_toboolean(state, 2));
   node->setSpriteVisible(id, visible);

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

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   float x = 0.0f, y = 0.0f;
   std::vector<DetonationAnimation::DetonationRing> rings;

   if (argc == 2)
   {
      x = static_cast<float>(lua_tonumber(state, 1));
      y = static_cast<float>(lua_tonumber(state, 2));
   }

   constexpr auto detonation_ring_param_count = 7;
   if (argc % detonation_ring_param_count == 0)
   {
      // 1: detonation_count (int)
      // 2: center_x (float)
      // 3: center_y (float)
      // 4: radius (float)
      // 5: speed_variance (float)
      // 6: variance_pos_x (float)
      // 7: variance_pos_y (float)

      for (auto i = 0; i < argc / detonation_ring_param_count; i++)
      {
         DetonationAnimation::DetonationRing ring;
         int32_t index = i * detonation_ring_param_count;

         ring._detonation_count = static_cast<int32_t>(lua_tointeger(state, index + 1));
         ring._center.x = static_cast<float>(lua_tonumber(state, index + 2));
         ring._center.y = static_cast<float>(lua_tonumber(state, index + 3));
         ring._radius = static_cast<float>(lua_tonumber(state, index + 4));
         ring._variance_animation_speed = static_cast<float>(lua_tonumber(state, index + 5));
         ring._variance_position.x = static_cast<float>(lua_tonumber(state, index + 6));
         ring._variance_position.y = static_cast<float>(lua_tonumber(state, index + 7));

         rings.push_back(ring);
      }
   }

   node->playDetonationAnimationFromScript(x, y, rings);

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
 *    param 3: x offset
 *    param 4: y offset
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
 * @brief addShapeRect add a rectangular shape to the node
 * @param state lua state
 *    param 1: rect width
 *    param 2: rect height
 *    param 3: amount of bevel
 *    param 4: x offset
 *    param 5: y offset
 * @return error code
 */
int32_t addShapeRectBevel(lua_State* state)
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

   const auto width = static_cast<float>(lua_tonumber(state, 1));
   const auto height = static_cast<float>(lua_tonumber(state, 2));
   const auto bevel = static_cast<float>(lua_tonumber(state, 3));
   const auto offset_x = static_cast<float>(lua_tonumber(state, 4));
   const auto offset_y = static_cast<float>(lua_tonumber(state, 5));

   node->addShapeRectBevel(width, height, bevel, offset_x, offset_y);

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
   if (argc < 4)
   {
      Log::Error() << "bad parameters for addWeapon";
      exit(1);
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto weapon_type = static_cast<WeaponType>(lua_tointeger(state, 1));
   const auto fire_interval = static_cast<int>(lua_tointeger(state, 2));
   const auto damage_value = static_cast<int>(lua_tointeger(state, 3));
   const auto gravity_scale = static_cast<float>(lua_tonumber(state, 4));

   // add weapon with polygon projectile shape
   std::vector<b2Vec2> polygon_points;
   if (argc >= 6 && ((argc - 6) % 2 == 0))
   {
      constexpr auto parameter_count = 2u;
      for (auto i = parameter_count + 1; i < argc - parameter_count; i += 2u)
      {
         const auto x = static_cast<float>(lua_tonumber(state, i));
         const auto y = static_cast<float>(lua_tonumber(state, i + 1));
         polygon_points.emplace_back(x, y);
      }
   }

   // add weapon with projectile radius only
   const auto radius = argc == 5 ? static_cast<float>(lua_tonumber(state, 5)) : 0.0f;

   node->addWeaponFromScript(weapon_type, fire_interval, damage_value, gravity_scale, radius, polygon_points);

   return 0;
}

/**
 * @brief useWeapon fire a weapon
 * @param state lua state
 *    param 1: index of the weapon
 *    param 2: x position where the shot comes from
 *    param 3: y position where the shot comes from
 *    param 4: x direction
 *    param 5: y direction
 * @return error code
 */
int32_t useWeapon(lua_State* state)
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
   node->useWeapon(index, {pos_x, pos_y}, {dir_x, dir_y});

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

      rect.position.x = x1;
      rect.position.y = y1;
      rect.size.x = width;
      rect.size.y = height;
   }

   if (valid)
   {
      node->setProjectileTexture(index, path, rect);
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

   node->setProjectileAnimation(
      weapon_index,
      path.string(),
      frame_width,
      frame_height,
      frame_origin_x,
      frame_origin_y,
      time_per_frame_s,
      frame_count,
      frames_per_row,
      start_frame
   );

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

   node->startTimer(delay, timer_id);

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
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->addSample(sample);
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

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   node->playSample(sample, volume);

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
   Log::Info() << message;

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

   node->registerHitAnimation(
      weapon_index,
      path.string(),
      frame_width,
      frame_height,
      time_per_frame_s,
      frame_count,
      frames_per_row,
      start_frame
   );

   return 0;
}

/**
 * @brief registerHitSamples register a hit animation for a given weapon
 * @param state lua state
 *    param 1: animation path / identifier
 *    param 2: sample 1
 *    param n: sample n
 * @return error code
 */
int32_t registerHitSamples(lua_State* state)
{
   const auto argc = lua_gettop(state);
   if (argc < 3)
   {
      return 0;
   }

   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto path = lua_tostring(state, 1);

   std::vector<std::pair<std::string, float>> samples;
   for (auto index = 2; index <= argc; index++)
   {
      const auto sample_path = lua_tostring(state, index);

      auto volume = 1.0f;
      if (index + 1 <= argc)
      {
         if (lua_isnumber(state, index + 1))
         {
            volume = static_cast<float>(lua_tonumber(state, index + 1));
            index++;
         }
      }

      samples.emplace_back(sample_path, volume);
   }

   node->registerHitSamples(path, samples);
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
   node->setKeysPressed(keys_pressed);

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

   node->die();
   return 0;
}

/**
 * @brief addPlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t addPlayerSkill(lua_State* state)
{
   auto node = OBJINSTANCE;
   if (!node)
   {
      return 0;
   }

   const auto argc = lua_gettop(state);
   if (argc != 1)
   {
      return 0;
   }

   const auto skill = static_cast<int32_t>(lua_tointeger(state, 1));

   node->addPlayerSkill(skill);
   return 0;
}

/**
 * @brief removePlayerSkill add a skill to the player
 * @param state lua state
 *    param 1: skill to add
 * @return error code
 */
int32_t removePlayerSkill(lua_State* state)
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

   const auto skill = static_cast<int32_t>(lua_tointeger(state, 1));

   node->removePlayerSkill(skill);
   return 0;
}

[[noreturn]] void error(lua_State* state, const char* /*scope*/)
{
   // the error message is on top of the stack.
   // fetch it, print32_t it and then pop it off the stack.
   std::stringstream os;
   os << lua_tostring(state, -1);

   Log::Error() << os.str();

   lua_pop(state, 1);

   exit(1);
}

}  // namespace LuaNodeCallbacks
