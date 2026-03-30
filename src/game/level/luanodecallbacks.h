#pragma once

#include <cstdint>

struct lua_State;

/// \brief lua-exposed callbacks that drive LuaNode gameplay behavior.
namespace LuaNodeCallbacks
{

// properties
/// \brief copies script key-value pairs into the node property map and applies side effects.
/// \param state active lua state that contains the property table.
/// \return number of lua return values pushed to the stack.
int32_t updateProperties(lua_State* state);

// hitboxes and debug
/// \brief adds a player-damage hitbox relative to the scripted enemy.
/// \param state active lua state containing hitbox position and size in pixels.
/// \return number of lua return values pushed to the stack.
int32_t addHitbox(lua_State* state);

/// \brief allocates one debug rectangle slot for runtime visualization.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t addDebugRect(lua_State* state);

/// \brief updates an existing debug rectangle used by enemy scripts.
/// \param state active lua state with rectangle index and geometry.
/// \return number of lua return values pushed to the stack.
int32_t updateDebugRect(lua_State* state);

// audio
/// \brief configures distance-based volume interpolation for this node.
/// \param state active lua state with far and near range settings.
/// \return number of lua return values pushed to the stack.
int32_t addAudioRange(lua_State* state);

/// \brief selects how the node updates positional audio (always, range-based, or room-based).
/// \param state active lua state with an AudioUpdateBehavior enum value.
/// \return number of lua return values pushed to the stack.
int32_t setAudioUpdateBehavior(lua_State* state);

/// \brief sets fallback sample volume when no audio range is configured.
/// \param state active lua state with the reference volume value.
/// \return number of lua return values pushed to the stack.
int32_t setReferenceVolume(lua_State* state);

/// \brief preloads a named sample for later playback by the node.
/// \param state active lua state with the sample identifier.
/// \return number of lua return values pushed to the stack.
int32_t addSample(lua_State* state);

/// \brief plays a previously known sample at the requested volume.
/// \param state active lua state with sample identifier and volume.
/// \return number of lua return values pushed to the stack.
int32_t playSample(lua_State* state);

// sprites
/// \brief changes the texture rectangle of one sprite layer.
/// \param state active lua state with sprite id and texture rectangle in pixels.
/// \return number of lua return values pushed to the stack.
int32_t updateSpriteRect(lua_State* state);

/// \brief tints one sprite layer with rgba color channels.
/// \param state active lua state with sprite id and color channels.
/// \return number of lua return values pushed to the stack.
int32_t setSpriteColor(lua_State* state);

/// \brief sets x and y scale factors for one sprite layer.
/// \param state active lua state with sprite id and scale values.
/// \return number of lua return values pushed to the stack.
int32_t setSpriteScale(lua_State* state);

/// \brief appends an additional sprite layer to the node.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t addSprite(lua_State* state);

/// \brief sets the pivot point used for rotation and scaling of one sprite layer.
/// \param state active lua state with sprite id and origin coordinates.
/// \return number of lua return values pushed to the stack.
int32_t setSpriteOrigin(lua_State* state);

/// \brief sets local pixel offset for one sprite layer relative to node position.
/// \param state active lua state with sprite id and offset values.
/// \return number of lua return values pushed to the stack.
int32_t setSpriteOffset(lua_State* state);

/// \brief toggles visibility of one sprite layer without removing it.
/// \param state active lua state with sprite id and visibility flag.
/// \return number of lua return values pushed to the stack.
int32_t setSpriteVisible(lua_State* state);

// physics queries
/// \brief checks whether a script-provided rectangle overlaps the player bounds.
/// \param state active lua state with rectangle coordinates in pixels.
/// \return number of lua return values pushed to the stack.
int32_t intersectsWithPlayer(lua_State* state);

/// \brief reports whether the player is currently dead.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t isPlayerDead(lua_State* state);

/// \brief runs a box2d aabb query and returns hit count.
/// \param state active lua state with query bounds in pixels.
/// \return number of lua return values pushed to the stack.
int32_t queryAABB(lua_State* state);

/// \brief runs a box2d ray cast and returns hit count.
/// \param state active lua state with ray start and end coordinates in pixels.
/// \return number of lua return values pushed to the stack.
int32_t queryRayCast(lua_State* state);

/// \brief tests the physics occupancy grid for line-of-sight between two points.
/// \param state active lua state with path endpoints in pixels.
/// \return number of lua return values pushed to the stack.
int32_t isPhsyicsPathClear(lua_State* state);

// body manipulation
/// \brief sets collision damage applied to the player by this node.
/// \param state active lua state with damage amount.
/// \return number of lua return values pushed to the stack.
int32_t setDamageToPlayer(lua_State* state);

/// \brief changes render depth of the node.
/// \param state active lua state with z layer value.
/// \return number of lua return values pushed to the stack.
int32_t setZIndex(lua_State* state);

/// \brief switches the node body type to dynamic.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t makeDynamic(lua_State* state);

/// \brief switches the node body type to static.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t makeStatic(lua_State* state);

/// \brief sets custom gravity scaling on the node body.
/// \param state active lua state with gravity scale.
/// \return number of lua return values pushed to the stack.
int32_t setGravityScale(lua_State* state);

/// \brief enables or disables the node body in box2d simulation.
/// \param state active lua state with enabled flag.
/// \return number of lua return values pushed to the stack.
int32_t setActive(lua_State* state);

/// \brief returns current linear velocity vector of the node body.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t getLinearVelocity(lua_State* state);

/// \brief returns world gravity value used by box2d.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t getGravity(lua_State* state);

/// \brief sets linear velocity of the node body.
/// \param state active lua state with velocity x and y.
/// \return number of lua return values pushed to the stack.
int32_t setLinearVelocity(lua_State* state);

/// \brief applies an impulse to the node body center.
/// \param state active lua state with impulse x and y.
/// \return number of lua return values pushed to the stack.
int32_t applyLinearImpulse(lua_State* state);

/// \brief applies a continuous force to the node body center.
/// \param state active lua state with force x and y.
/// \return number of lua return values pushed to the stack.
int32_t applyForce(lua_State* state);

/// \brief sets body position and rotation in world space.
/// \param state active lua state with x, y, and angle.
/// \return number of lua return values pushed to the stack.
int32_t setTransform(lua_State* state);

// visibility
/// \brief toggles overall node rendering visibility.
/// \param state active lua state with visibility flag.
/// \return number of lua return values pushed to the stack.
int32_t setVisible(lua_State* state);

// combat
/// \brief applies direct damage and knockback to the player.
/// \param state active lua state with damage amount and force direction.
/// \return number of lua return values pushed to the stack.
int32_t damage(lua_State* state);

/// \brief applies area damage around a script-defined center point.
/// \param state active lua state with damage amount, center, and radius.
/// \return number of lua return values pushed to the stack.
int32_t damageRadius(lua_State* state);

/// \brief triggers camera and scene boom effect at a world position.
/// \param state active lua state with boom center and intensity.
/// \return number of lua return values pushed to the stack.
int32_t boom(lua_State* state);

/// \brief plays scripted detonation visuals using either default or ring-based parameters.
/// \param state active lua state with either center coordinates or ring parameter groups.
/// \return number of lua return values pushed to the stack.
int32_t playDetonationAnimation(lua_State* state);

/// \brief destroys the node body and marks the scripted enemy as dead.
/// \param state active lua state.
/// \return number of lua return values pushed to the stack.
int32_t die(lua_State* state);

// shapes
/// \brief adds a circular fixture shape to the node body definition.
/// \param state active lua state with radius and center.
/// \return number of lua return values pushed to the stack.
int32_t addShapeCircle(lua_State* state);

/// \brief adds a rectangular fixture shape to the node body definition.
/// \param state active lua state with width, height, and offset.
/// \return number of lua return values pushed to the stack.
int32_t addShapeRect(lua_State* state);

/// \brief adds a beveled rectangle fixture shape to the node body definition.
/// \param state active lua state with width, height, bevel size, and offsets.
/// \return number of lua return values pushed to the stack.
int32_t addShapeRectBevel(lua_State* state);

/// \brief adds a polygon fixture shape to the node body definition.
/// \param state active lua state with alternating x and y coordinates.
/// \return number of lua return values pushed to the stack.
int32_t addShapePoly(lua_State* state);

// weapons
/// \brief creates a scripted weapon and attaches it to the node.
/// \param state active lua state with weapon type and projectile data.
/// \return number of lua return values pushed to the stack.
int32_t addWeapon(lua_State* state);

/// \brief fires one weapon instance from a position toward a direction.
/// \param state active lua state with weapon index, origin, and direction.
/// \return number of lua return values pushed to the stack.
int32_t useWeapon(lua_State* state);

/// \brief sets projectile sprite texture and optional sub-rectangle for one weapon.
/// \param state active lua state with weapon index, path, and optional rect.
/// \return number of lua return values pushed to the stack.
int32_t updateProjectileTexture(lua_State* state);

/// \brief configures projectile animation frames for one weapon.
/// \param state active lua state with animation texture and frame timing parameters.
/// \return number of lua return values pushed to the stack.
int32_t updateProjectileAnimation(lua_State* state);

/// \brief registers impact animation data for projectile collisions.
/// \param state active lua state with weapon index and animation parameters.
/// \return number of lua return values pushed to the stack.
int32_t registerHitAnimation(lua_State* state);

/// \brief registers impact samples and optional per-sample volumes for an animation id.
/// \param state active lua state with animation key and sample entries.
/// \return number of lua return values pushed to the stack.
int32_t registerHitSamples(lua_State* state);

// utilities
/// \brief starts a one-shot timer that calls timeout in the lua script.
/// \param state active lua state with delay and timer id.
/// \return number of lua return values pushed to the stack.
int32_t timer(lua_State* state);

/// \brief forwards a script log message to the engine logger.
/// \param state active lua state with message text.
/// \return number of lua return values pushed to the stack.
int32_t debug(lua_State* state);

/// \brief forwards current key bitmask to the scripted node.
/// \param state active lua state with KeyPressed flags.
/// \return number of lua return values pushed to the stack.
int32_t updateKeysPressed(lua_State* state);

// player interaction
/// \brief grants a player skill flag through the node script.
/// \param state active lua state with skill bitmask.
/// \return number of lua return values pushed to the stack.
int32_t addPlayerSkill(lua_State* state);

/// \brief removes a player skill flag through the node script.
/// \param state active lua state with skill bitmask.
/// \return number of lua return values pushed to the stack.
int32_t removePlayerSkill(lua_State* state);

// error handling
/// \brief logs lua error text and terminates execution.
/// \param state active lua state containing error message on top of the stack.
/// \param scope optional callback name used to add context to the log.
[[noreturn]] void error(lua_State* state, const char* scope = nullptr);

}  // namespace LuaNodeCallbacks
