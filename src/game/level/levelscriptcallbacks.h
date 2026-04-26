#pragma once

#include <cstdint>

struct lua_State;

/// \brief lua-exposed callbacks that drive LevelScript gameplay behavior.
namespace LevelScriptCallbacks
{

// collision
int32_t addCollisionRect(lua_State* state);
int32_t addSensorRectCallback(lua_State* state);
int32_t isPlayerIntersectingSensorRect(lua_State* state);

// mechanisms
int32_t isMechanismEnabled(lua_State* state);
int32_t isMechanismVisible(lua_State* state);
int32_t setMechanismEnabled(lua_State* state);
int32_t setMechanismVisible(lua_State* state);
int32_t flashMechanism(lua_State* state);
int32_t toggle(lua_State* state);
int32_t showDialogue(lua_State* state);

// player skills / stats
int32_t addPlayerSkill(lua_State* state);
int32_t removePlayerSkill(lua_State* state);
int32_t addPlayerHealth(lua_State* state);
int32_t addPlayerHealthMax(lua_State* state);

// inventory
int32_t inventoryAdd(lua_State* state);
int32_t inventoryRemove(lua_State* state);
int32_t inventoryHas(lua_State* state);

// weapons
int32_t giveWeaponBow(lua_State* state);
int32_t giveWeaponGun(lua_State* state);
int32_t giveWeaponSword(lua_State* state);

// lua nodes
int32_t writeLuaNodeProperty(lua_State* state);
int32_t setLuaNodeVisible(lua_State* state);
int32_t setLuaNodeActive(lua_State* state);

// audio / camera / scene
int32_t playMusic(lua_State* state);
int32_t lockPlayerControls(lua_State* state);
int32_t setAmbient(lua_State* state);
int32_t setZoomFactor(lua_State* state);
int32_t playEventRecording(lua_State* state);

// utilities
int32_t debug(lua_State* state);
int32_t translate(lua_State* state);

// error handling
/// \brief logs lua error text and terminates execution.
/// \param state active lua state containing error message on top of the stack.
/// \param scope optional callback name used to add context to the log.
[[noreturn]] void error(lua_State* state, const char* scope = nullptr);

}  // namespace LevelScriptCallbacks
