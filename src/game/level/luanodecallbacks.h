#pragma once

#include <cstdint>

struct lua_State;

/*! \brief Lua callback functions for LuaNode
 *
 * This module contains all static Lua callback functions that are registered
 * with the Lua state. These functions bridge Lua scripts and C++ game logic.
 */
namespace LuaNodeCallbacks
{

// properties
int32_t updateProperties(lua_State* state);

// hitboxes and debug
int32_t addHitbox(lua_State* state);
int32_t addDebugRect(lua_State* state);
int32_t updateDebugRect(lua_State* state);

// audio
int32_t addAudioRange(lua_State* state);
int32_t setAudioUpdateBehavior(lua_State* state);
int32_t setReferenceVolume(lua_State* state);
int32_t addSample(lua_State* state);
int32_t playSample(lua_State* state);

// sprites
int32_t updateSpriteRect(lua_State* state);
int32_t setSpriteColor(lua_State* state);
int32_t setSpriteScale(lua_State* state);
int32_t addSprite(lua_State* state);
int32_t setSpriteOrigin(lua_State* state);
int32_t setSpriteOffset(lua_State* state);
int32_t setSpriteVisible(lua_State* state);

// physics queries
int32_t intersectsWithPlayer(lua_State* state);
int32_t isPlayerDead(lua_State* state);
int32_t queryAABB(lua_State* state);
int32_t queryRayCast(lua_State* state);
int32_t isPhsyicsPathClear(lua_State* state);

// body manipulation
int32_t setDamageToPlayer(lua_State* state);
int32_t setZIndex(lua_State* state);
int32_t makeDynamic(lua_State* state);
int32_t makeStatic(lua_State* state);
int32_t setGravityScale(lua_State* state);
int32_t setActive(lua_State* state);
int32_t getLinearVelocity(lua_State* state);
int32_t getGravity(lua_State* state);
int32_t setLinearVelocity(lua_State* state);
int32_t applyLinearImpulse(lua_State* state);
int32_t applyForce(lua_State* state);
int32_t setTransform(lua_State* state);

// visibility
int32_t setVisible(lua_State* state);

// combat
int32_t damage(lua_State* state);
int32_t damageRadius(lua_State* state);
int32_t boom(lua_State* state);
int32_t playDetonationAnimation(lua_State* state);
int32_t die(lua_State* state);

// shapes
int32_t addShapeCircle(lua_State* state);
int32_t addShapeRect(lua_State* state);
int32_t addShapeRectBevel(lua_State* state);
int32_t addShapePoly(lua_State* state);

// weapons
int32_t addWeapon(lua_State* state);
int32_t useWeapon(lua_State* state);
int32_t updateProjectileTexture(lua_State* state);
int32_t updateProjectileAnimation(lua_State* state);
int32_t registerHitAnimation(lua_State* state);
int32_t registerHitSamples(lua_State* state);

// utilities
int32_t timer(lua_State* state);
int32_t debug(lua_State* state);
int32_t updateKeysPressed(lua_State* state);

// player interaction
int32_t addPlayerSkill(lua_State* state);
int32_t removePlayerSkill(lua_State* state);

// error handling
[[noreturn]] void error(lua_State* state, const char* scope = nullptr);

}  // namespace LuaNodeCallbacks
