#pragma once

#include "game/constants.h"
#include "game/level/chunk.h"
#include "game/player/playerbelt.h"
#include "game/player/playerbend.h"
#include "game/player/playercontrols.h"
#include "game/player/playereyepositions.h"
#include "game/player/playerjump.h"
#include "game/player/playerplatform.h"

#include <SFML/Graphics.hpp>
#include "box2d/box2d.h"

#include <memory>
#include <optional>

class PlayerAnimation;

/// \brief abstract interface for the player character; used by mechanisms and systems that must
///        interact with the player without depending on the concrete Player implementation.
class PlayerInterface
{
public:
   virtual ~PlayerInterface() = default;

   /// \brief gets current player position in pixels with floating-point precision.
   /// \return pixel-space player position.
   virtual const sf::Vector2f& getPixelPositionFloat() const = 0;

   /// \brief gets current player position in integer pixel coordinates.
   /// \return pixel-space player position rounded to integers.
   virtual const sf::Vector2i& getPixelPositionInt() const = 0;

   /// \brief gets current float hitbox rectangle in pixel space.
   /// \return player rectangle in pixels.
   virtual const sf::FloatRect& getPixelRectFloat() const = 0;

   /// \brief gets current integer hitbox rectangle in pixel space.
   /// \return integer player rectangle in pixels.
   virtual const sf::IntRect& getPixelRectInt() const = 0;

   /// \brief computes the current foot sensor aabb in floating-point pixel coordinates.
   /// \return float pixel rectangle covering the foot sensor.
   virtual sf::FloatRect computeFootSensorPixelFloatRect() const = 0;

   /// \brief gets the player's box2d body.
   /// \return pointer to the dynamic body used for player physics.
   virtual b2Body* getBody() const = 0;

   /// \brief reports whether the player has no foot contacts and is not swimming.
   /// \return true when airborne.
   virtual bool isInAir() const = 0;

   /// \brief reports whether the player is currently inside a water atmosphere tile.
   /// \return true when in water.
   virtual bool isInWater() const = 0;

   /// \brief reports whether the player currently has ground foot contacts.
   /// \return true when at least one foot contact exists.
   virtual bool isOnGround() const = 0;

   /// \brief reports whether the player has entered dead state.
   /// \return true when dead.
   virtual bool isDead() const = 0;

   /// \brief reports whether the player currently faces right.
   /// \return true when orientation is right.
   virtual bool isPointingRight() const = 0;

   /// \brief reports whether the player currently faces left.
   /// \return true when orientation is left.
   virtual bool isPointingLeft() const = 0;

   /// \brief gets current chunk index for streaming and logic queries.
   /// \return reference to current chunk coordinates.
   virtual const Chunk& getChunk() const = 0;

   /// \brief gets the shared controls object used by the player.
   /// \return shared pointer reference to player controls.
   virtual const std::shared_ptr<PlayerControls>& getControls() const = 0;

   /// \brief gets jump subsystem state and logic wrapper.
   /// \return reference to player jump subsystem.
   virtual const PlayerJump& getJump() const = 0;

   /// \brief gets moving-platform helper state.
   /// \return reference to player platform subsystem.
   virtual PlayerPlatform& getPlatform() = 0;

   /// \brief gets conveyor-belt movement helper.
   /// \return reference to player belt subsystem.
   virtual PlayerBelt& getBelt() = 0;

   /// \brief applies damage and optional knockback impulse, with cooldown and invulnerability guards.
   /// \param damage_amount health points to subtract.
   /// \param force knockback vector in pixel units converted to physics impulse.
   virtual void damage(int32_t damage_amount, const sf::Vector2f& force = sf::Vector2f{0.0f, 0.0f}) = 0;

   /// \brief kills the player by forcing lethal damage and optionally overriding death reason.
   /// \param death_reason optional explicit death reason to persist for animation and logic.
   virtual void kill(std::optional<DeathReason> death_reason = std::nullopt) = 0;

   /// \brief sets player pixel position and teleports the box2d body to match.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   virtual void setBodyViaPixelPosition(float x, float y) = 0;

   /// \brief starts alpha fade-out animation for the player sprite.
   /// \param fade_out_speed_factor alpha decay speed multiplier per second.
   virtual void fadeOut(float fade_out_speed_factor = 5.0f) = 0;

   /// \brief stops fading and restores full player alpha.
   virtual void fadeOutReset() = 0;

   /// \brief stores the latest contact impulse value for hard-landing evaluation.
   /// \param intensity impulse intensity reported by contact resolution.
   virtual void impulse(float intensity) = 0;

   /// \brief stores the ground body currently supporting the player, used for slope analysis.
   /// \param body ground body reported by the physics contact listener.
   virtual void setGroundBody(b2Body* body) = 0;

   /// \brief gets the box2d fixture used as the foot contact sensor.
   /// \return pointer to the foot sensor fixture.
   virtual b2Fixture* getFootSensorFixture() const = 0;

   /// \brief reports whether the player is currently in a hard-landing stun.
   /// \return true when the hard-landing stun is active.
   virtual bool isHardLanding() const = 0;

   /// \brief gets bend and crouch state.
   /// \return reference to the player bend subsystem.
   virtual const PlayerBend& getBend() const = 0;

   /// \brief gets eye position data for the current animation cycle.
   /// \return reference to the eye positions instance.
   virtual const PlayerEyePositions& getEyePositions() const = 0;

   /// \brief gets the animation controller for the player.
   /// \return shared pointer to the player animation instance.
   virtual const std::shared_ptr<PlayerAnimation>& getPlayerAnimation() const = 0;
};
