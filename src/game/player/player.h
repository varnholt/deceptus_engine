#pragma once

#include "game/animation/animationpool.h"
#include "game/constants.h"
#include "game/effects/waterbubbles.h"
#include "game/level/chunk.h"
#include "game/level/gamenode.h"
#include "game/player/itemsystem.h"
#include "game/player/playeranimation.h"
#include "game/player/playerattack.h"
#include "game/player/playerattackdash.h"
#include "game/player/playerbelt.h"
#include "game/player/playerbend.h"
#include "game/player/playerclimb.h"
#include "game/player/playercontrols.h"
#include "game/player/playerdash.h"
#include "game/player/playerdive.h"
#include "game/player/playerjump.h"
#include "game/player/playerjumptrace.h"
#include "game/player/playerplatform.h"
#include "game/player/playerspeed.h"
#include "game/player/weaponsystem.h"

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <memory>

class Animation;
class GameContactListener;
struct ScreenTransition;
class Weapon;
struct WeaponSystem;
class ItemSystem;

/// \brief main player entity that coordinates rendering, physics, combat, input, and progression state.
class Player : public GameNode
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using ToggleCallback = std::function<void()>;

   /// \brief animation snapshot used to render dash motion blur trails.
   struct PositionedAnimation
   {
      sf::Vector2f _position;
      std::shared_ptr<Animation> _animation;
   };

public:
   /// \brief constructs the player node, initializes subsystems, and registers shared player state.
   /// \param parent optional parent game node.
   Player(GameNode* parent = nullptr);
   /// \brief destroys the player instance.
   virtual ~Player() = default;

   /// \brief gets the globally tracked current player instance.
   /// \return pointer to the active player singleton.
   static Player* getCurrent();

   /// \brief wires callbacks and input handlers that require fully constructed subsystems.
   void initialize();
   /// \brief creates physics fixtures and places the player at the level start position.
   void initializeLevel();
   /// \brief registers controller button callbacks for jump, dash, and inventory slots.
   void initializeController();
   /// \brief draws player animations, equipped gear, and related visual effects.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal);
   /// \brief draws a translucent stencil version of current player animations.
   /// \param color color render target.
   void drawStencil(sf::RenderTarget& color);

   /// \brief runs one full frame of player simulation, including physics interaction, audio, and animations.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);
   /// \brief reloads animation assets and rebinds player animation cycles.
   void reloadAnimationPool();

   /// \brief marks the player as dead, plays death audio, and persists death statistics.
   void die();
   /// \brief resets player runtime state for respawn at the current level start or checkpoint.
   void reset();
   /// \brief evaluates the current death reason from contacts, velocity, health, and external overrides.
   /// \return detected death reason or invalid when alive.
   DeathReason checkDead() const;

   /// \brief reports whether the player currently faces right.
   /// \return true when orientation is right.
   bool isPointingRight() const;
   /// \brief reports whether the player currently faces left.
   /// \return true when orientation is left.
   bool isPointingLeft() const;

   /// \brief sets the logical start pixel position used before spawning or resets.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   void setStartPixelPosition(float x, float y);

   /// \brief gets current player position in pixels with floating-point precision.
   /// \return pixel-space player position.
   const sf::Vector2f& getPixelPositionFloat() const;
   /// \brief gets current player position in integer pixel coordinates.
   /// \return pixel-space player position rounded to integers.
   const sf::Vector2i& getPixelPositionInt() const;
   /// \brief sets cached pixel position values.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   void setPixelPosition(float x, float y);

   /// \brief gets current float hitbox rectangle in pixel space.
   /// \return player rectangle in pixels.
   const sf::FloatRect& getPixelRectFloat() const;
   /// \brief gets current integer hitbox rectangle in pixel space.
   /// \return integer player rectangle in pixels.
   const sf::IntRect& getPixelRectInt() const;

   /// \brief gets the player's box2d body.
   /// \return pointer to the dynamic body used for player physics.
   b2Body* getBody() const;
   /// \brief gets the dedicated foot sensor fixture.
   /// \return pointer to the fixture used for foot area checks.
   b2Fixture* getFootSensorFixture() const;
   /// \brief computes the current foot sensor aabb in integer pixel coordinates.
   /// \return integer pixel rectangle covering the foot sensor.
   sf::IntRect computeFootSensorPixelIntRect() const;
   /// \brief computes the current foot sensor aabb in floating-point pixel coordinates.
   /// \return float pixel rectangle covering the foot sensor.
   sf::FloatRect computeFootSensorPixelFloatRect() const;

   /// \brief assigns the box2d world used to create and update player fixtures.
   /// \param world box2d world instance.
   void setWorld(const std::shared_ptr<b2World>& world);
   /// \brief clears the world reference.
   void resetWorld();
   /// \brief synchronizes cached pixel position from the box2d body transform.
   void updatePixelPosition();
   /// \brief stores previous body position and velocity for collision-impact analysis.
   void updatePreviousBodyState();
   /// \brief recomputes cached player pixel-space rectangles from current position.
   void updatePixelRect();
   /// \brief refreshes current world chunk based on player pixel position.
   void updateChunk();
   /// \brief sets player pixel position and teleports the box2d body to match.
   /// \param x x coordinate in pixels.
   /// \param y y coordinate in pixels.
   void setBodyViaPixelPosition(float x, float y);
   /// \brief applies a friction value to all player fixtures and resets contact friction caches.
   /// \param f friction coefficient to apply.
   void setFriction(float f);
   /// \brief gets horizontal velocity normalized by the current context-dependent max speed.
   /// \return normalized horizontal velocity where 1.0 equals current max speed.
   float getVelocityNormalized() const;

   /// \brief reports whether player rendering is enabled.
   /// \return true when draw calls should render the player.
   bool getVisible() const;
   /// \brief enables or disables player rendering.
   /// \param visible true to draw it.
   void setVisible(bool visible);
   /// \brief starts alpha fade-out animation for the player sprite.
   /// \param fade_out_speed_factor alpha decay speed multiplier per second.
   void fadeOut(float fade_out_speed_factor = 5.0f);
   /// \brief stops fading and restores full player alpha.
   void fadeOutReset();

   /// \brief sets the currently contacted ground body used for slope analysis.
   /// \param body ground body currently supporting the player.
   void setGroundBody(b2Body* body);

   /// \brief reports whether the player has no foot contacts and is not swimming.
   /// \return true when airborne.
   bool isInAir() const;
   /// \brief reports whether the player is currently inside a water atmosphere tile.
   /// \return true when in water.
   bool isInWater() const;
   /// \brief reports whether the player currently has ground foot contacts.
   /// \return true when at least one foot contact exists.
   bool isOnGround() const;
   /// \brief reports whether the player has entered dead state.
   /// \return true when dead.
   bool isDead() const;
   /// \brief determines whether upward motion currently passes through one-way wall contacts.
   /// \return true when moving upward while touching one-way walls.
   bool isJumpingThroughOneWayWall();

   /// \brief stores water-state flag for movement and animation systems.
   /// \param inWater true when the player is inside water.
   void setInWater(bool inWater);

   /// \brief gets the player's rendering z index.
   /// \return current z index.
   int getZIndex() const;
   /// \brief sets the player's rendering z index.
   /// \param z z index.
   void setZIndex(int32_t z);

   /// \brief gets the player identifier.
   /// \return numeric player id.
   int getId() const;

   /// \brief stores the latest contact impulse value for hard-landing evaluation.
   /// \param intensity impulse intensity reported by contact resolution.
   void impulse(float intensity);
   /// \brief applies damage and optional knockback impulse, with cooldown and invulnerability guards.
   /// \param damage health points to subtract.
   /// \param force knockback vector in pixel units converted to physics impulse.
   void damage(int32_t damage, const sf::Vector2f& force = sf::Vector2f{0.0f, 0.0f});
   /// \brief kills the player by forcing lethal damage and optionally overriding death reason.
   /// \param death_reason optional explicit death reason to persist for animation and logic.
   void kill(std::optional<DeathReason> death_reason = std::nullopt);

   /// \brief gets the shared controls object used by the player.
   /// \return shared pointer reference to player controls.
   const std::shared_ptr<PlayerControls>& getControls() const;
   /// \brief gets jump subsystem state and logic wrapper.
   /// \return reference to player jump subsystem.
   const PlayerJump& getJump() const;
   /// \brief gets bend and crouch state.
   /// \return reference to player bend subsystem.
   const PlayerBend& getBend() const;
   /// \brief gets conveyor-belt movement helper.
   /// \return reference to player belt subsystem.
   PlayerBelt& getBelt();
   /// \brief gets moving-platform helper state.
   /// \return reference to player platform subsystem.
   PlayerPlatform& getPlatform();
   /// \brief gets current chunk index for streaming and logic queries.
   /// \return reference to current chunk coordinates.
   const Chunk& getChunk() const;

   /// \brief sets a generic toggle callback used by external systems.
   /// \param callback callback invoked by toggle-related logic.
   void setToggleCallback(const ToggleCallback& callback);

private:
   /// \brief creates all player physics fixtures in the assigned box2d world.
   void createPlayerBody();

   /// \brief advances fade-out alpha while fading is active.
   /// \param dt elapsed frame time.
   void updateFadeOut(const sf::Time& dt);
   /// \brief builds animation context and advances animation state machines.
   /// \param dt elapsed frame time.
   void updateAnimation(const sf::Time& dt);
   /// \brief refreshes water entry/exit state, gravity settings, splash effects, and swim velocity cleanup.
   void updateAtmosphere();
   /// \brief refreshes bend/crouch state, crouch collision masks, and kneel audio triggers.
   void updateBendDown();
   /// \brief refreshes dash logic and optionally starts a dash in the requested direction.
   /// \param dir dash direction for this tick, or none for sustain-only updates.
   void updateDash(Dash dir = Dash::None);
   /// \brief refreshes weapon attack input and triggers weapon-specific attack behavior.
   void updateAttack();
   /// \brief advances attack-dash force burst applied after successful sword attacks.
   /// \param dt elapsed frame time.
   void updateAttackDash(const sf::Time& dt);
   /// \brief plays alternating footstep sounds based on grounded movement speed.
   void updateFootsteps();
   /// \brief raycasts the current ground body to refresh slope normal under the player.
   void updateGroundAngle();
   /// \brief advances hard-landing stun state and clears it when safe conditions are met.
   void updateHardLanding();
   /// \brief consumes stored contact impulse to trigger hard-landing effects and optional damage.
   void updateImpulse();
   /// \brief triggers one-way platform drop-through when matching input is active.
   void updateOneWayWallDrop();
   /// \brief applies collision-position corrections for chain-shape hiccup cases.
   void updateChainShapeCollisions();
   /// \brief refreshes facing direction from control orientation output.
   void updateOrientation();
   /// \brief refreshes portal-related logic.
   void updatePortal();
   /// \brief computes desired movement velocity and applies horizontal impulses and speed clamping.
   void updateVelocity();
   /// \brief refreshes all owned weapons with current frame context.
   /// \param dt elapsed frame time.
   void updateWeapons(const sf::Time& dt);
   /// \brief refreshes equipped item system behavior.
   /// \param dt elapsed frame time.
   void updateItems(const sf::Time& dt);
   /// \brief pushes current movement state into the jump subsystem and refreshes jump behavior.
   void updateJump();
   /// \brief refreshes wall-slide dust animation positioning and playback.
   /// \param dt elapsed frame time.
   void updateWallslide(const sf::Time& dt);
   /// \brief refreshes ambient water bubble effects around the player.
   /// \param dt elapsed frame time.
   void updateWaterBubbles(const sf::Time& dt);
   /// \brief refreshes spawn reveal timing, orientation lock, and reveal audio cue.
   void updateSpawn();
   /// \brief advances health and stamina regeneration/drain timers.
   /// \param dt elapsed frame time.
   void updateHealth(const sf::Time& dt);

   /// \brief enters hard-landing state, triggers camera boom, controller rumble, and grunt audio.
   void startHardLanding();
   /// \brief clears cached dash trail frames and restores animation alpha values.
   void resetMotionBlur();

   /// \brief creates the main player body fixture and stores fixture metadata.
   void createBody();
   /// \brief creates foot, sensor, and wall-slide helper fixtures.
   void createFeet();
   /// \brief switches collision mask bits between standing and crouching profiles.
   /// \param enabled true to enable the feature.
   void setMaskBitsCrouching(bool enabled);

   /// \brief gets movement speed cap for the current movement medium and state.
   /// \return maximum horizontal speed in meters per second.
   float getMaxVelocity() const;
   /// \brief computes desired horizontal velocity from controller analog input.
   /// \param speed speed context containing current velocity and accel/decel parameters.
   /// \return desired horizontal velocity based on controller input.
   float readVelocityFromController(const PlayerSpeed& speed) const;
   /// \brief computes desired horizontal velocity from keyboard movement keys.
   /// \param speed speed context containing current velocity and accel/decel parameters.
   /// \return desired horizontal velocity based on keyboard input.
   float readVelocityFromKeyboard(const PlayerSpeed& speed) const;
   /// \brief chooses input source and computes desired horizontal velocity from provided speed context.
   /// \param speed speed context containing current velocity and accel/decel parameters.
   /// \return desired horizontal velocity for this frame.
   float readDesiredVelocity(const PlayerSpeed& speed) const;
   /// \brief builds speed context from runtime state and computes desired horizontal velocity.
   /// \return desired horizontal velocity for this frame.
   float readDesiredVelocity() const;
   /// \brief gets context-specific deceleration constant.
   /// \return deceleration factor for ground, air, or water.
   float getDeceleration() const;
   /// \brief gets context-specific acceleration constant.
   /// \return acceleration factor for ground, air, or water.
   float getAcceleration() const;

   /// \brief logs jump trajectory samples for jump-tuning diagnostics.
   void traceJumpCurve();
   /// \brief handles high-level key actions such as jump, dash, and inventory use.
   /// \param key lookup key.
   void keyPressed(sf::Keyboard::Key key);

   /// \brief renders accumulated dash trail frames and appends current frame while dashing.
   /// \param color color render target.
   /// \param current_cycle currently selected player animation cycle.
   /// \param draw_position_px current draw position in pixels.
   void drawDash(sf::RenderTarget& color, const std::shared_ptr<Animation>& current_cycle, const sf::Vector2f& draw_position_px);
   /// \brief determines whether hurt-flash logic should skip rendering this frame.
   /// \return true when flashing logic requests draw suppression.
   bool checkDamageDrawSkip() const;
   /// \brief tints the current animation red based on recent damage time.
   /// \param current_cycle animation cycle to recolor.
   void updateHurtColor(const std::shared_ptr<Animation>& current_cycle);

   /// \brief uses an inventory slot when control state allows inventory interaction.
   /// \param slot inventory slot index.
   void useInventory(int32_t slot);

   // all related to player physics and box2d
   std::shared_ptr<b2World> _world;
   b2Body* _body = nullptr;
   static constexpr int32_t __foot_count = 4u;
   b2Fixture* _body_fixture = nullptr;
   b2Fixture* _foot_fixture[__foot_count]{nullptr, nullptr, nullptr, nullptr};
   b2Fixture* _foot_sensor_fixture = nullptr;
   b2Body* _ground_body = nullptr;
   b2Vec2 _ground_normal;
   b2Vec2 _position_previous;
   b2Vec2 _velocity_previous;
   float _impulse{0.0f};

   sf::Vector2f _pixel_position_f;
   sf::Vector2i _pixel_position_i;
   sf::FloatRect _pixel_rect_f;
   sf::IntRect _pixel_rect_i;

   sf::Time _time;
   sf::Clock _clock;
   sf::Clock _damage_clock;
   bool _damage_initialized{false};

   bool _points_to_left{false};
   bool _visible{true};
   bool _fade_out{false};
   float _fade_out_alpha{1.0f};
   float _fade_out_speed_factor{10.0f};
   bool _dead{false};
   bool _spawn_complete{false};
   bool _spawn_orientation_locked{false};
   std::optional<DeathReason> _death_reason;

   bool _in_water{false};
   HighResTimePoint _water_entered_time;

   float _next_footstep_time{0.0f};
   int32_t _step_counter{0};

   int32_t _z_index{0};
   int32_t _id{0};

   bool _hard_landing{false};
   int32_t _hard_landing_cycles{0};
   HighResTimePoint _timepoint_hard_landing;

   std::shared_ptr<PlayerControls> _controls;
   PlayerAttack _attack;
   PlayerAttackDash _attack_dash;
   PlayerBelt _belt;
   PlayerBend _bend;
   PlayerClimb _climb;
   PlayerDash _dash;
   PlayerJump _jump;
   PlayerJumpTrace _jump_trace;
   PlayerPlatform _platform;
   PlayerDive _dive;
   WaterBubbles _water_bubbles;

   std::shared_ptr<PlayerAnimation> _player_animation;
   std::deque<PositionedAnimation> _last_animations;

   Chunk _chunk{0, 0};
   AnimationPool _animation_pool{"data/sprites/animations.json"};

   static Player* __current;
};
