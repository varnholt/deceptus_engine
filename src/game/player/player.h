#pragma once

#include "game/animationpool.h"
#include "game/chunk.h"
#include "game/constants.h"
#include "game/gamenode.h"
#include "game/player/playeranimation.h"
#include "game/player/playerattack.h"
#include "game/player/playerbelt.h"
#include "game/player/playerbend.h"
#include "game/player/playerclimb.h"
#include "game/player/playercontrols.h"
#include "game/player/playerdash.h"
#include "game/player/playerjump.h"
#include "game/player/playerjumptrace.h"
#include "game/player/playerspeed.h"
#include "game/waterbubbles.h"

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <set>

class Animation;
class GameContactListener;
struct ScreenTransition;
class Weapon;
struct WeaponSystem;

class Player : public GameNode
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using ToggleCallback = std::function<void()>;

   struct PositionedAnimation
   {
      sf::Vector2f _position;
      std::shared_ptr<Animation> _animation;
   };

public:
   Player(GameNode* parent = nullptr);
   virtual ~Player() = default;

   static Player* getCurrent();

   void initialize();
   void initializeLevel();
   void initializeController();
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal);
   void drawStencil(sf::RenderTarget& color);

   void update(const sf::Time& dt);
   void reloadAnimationPool();

   void attack();
   void die();
   void reset();
   DeathReason checkDead() const;

   bool isPointingRight() const;
   bool isPointingLeft() const;

   void setStartPixelPosition(float x, float y);

   const sf::Vector2f& getPixelPositionFloat() const;
   const sf::Vector2i& getPixelPositionInt() const;
   void setPixelPosition(float x, float y);

   const sf::FloatRect& getPixelRectFloat() const;
   const sf::IntRect& getPixelRectInt() const;

   b2Body* getBody() const;
   b2Fixture* getFootSensorFixture() const;
   sf::IntRect computeFootSensorPixelIntRect() const;
   sf::FloatRect computeFootSensorPixelFloatRect() const;

   void setWorld(const std::shared_ptr<b2World>& world);
   void resetWorld();
   void updatePixelPosition();
   void updatePreviousBodyState();
   void updatePixelRect();
   void updateChunk();
   void setBodyViaPixelPosition(float x, float y);
   void setFriction(float f);

   bool getVisible() const;
   void setVisible(bool visible);
   void fadeOut(float fade_out_speed_factor = 5.0f);
   void fadeOutReset();

   void setPlatformBody(b2Body* body);
   b2Body* getPlatformBody() const;
   void setPlatformDx(float dx_px);
   void setGroundBody(b2Body* body);

   bool isInAir() const;
   bool isInWater() const;
   bool isOnPlatform() const;
   bool isOnGround() const;
   bool isDead() const;
   bool isJumpingThroughOneWayWall();

   void setInWater(bool inWater);

   int getZIndex() const;
   void setZIndex(int32_t z);

   int getId() const;

   void impulse(float intensity);
   void damage(int32_t damage, const sf::Vector2f& force = sf::Vector2f{0.0f, 0.0f});
   void kill(std::optional<DeathReason> death_reason = std::nullopt);
   void goToPortal(auto portal);

   const std::shared_ptr<WeaponSystem>& getWeaponSystem() const;
   const std::shared_ptr<PlayerControls>& getControls() const;
   const PlayerJump& getJump() const;
   const PlayerBend& getBend() const;
   PlayerBelt& getBelt();
   const Chunk& getChunk() const;

   void setToggleCallback(const ToggleCallback& callback);

private:
   void createPlayerBody();

   void updateFadeOut(const sf::Time& dt);
   void updateAnimation(const sf::Time& dt);
   void updateAtmosphere();
   void updateBendDown();
   void updateDash(Dash dir = Dash::None);
   void updateAttack();
   void updateFootsteps();
   void updateGroundAngle();
   void updateHardLanding();
   void updateImpulse();
   void updateOneWayWallDrop();
   void updateChainShapeCollisions();
   void updatePixelCollisions();
   void updatePlatformMovement(const sf::Time& dt);
   void updateOrientation();
   void updatePortal();
   void updateVelocity();
   void updateWeapons(const sf::Time& dt);
   void updateJump();
   void updateWallslide(const sf::Time& dt);
   void updateWaterBubbles(const sf::Time& dt);
   void updateSpawn();
   void updateHealth(const sf::Time& dt);

   void startHardLanding();
   void resetMotionBlur();

   void createBody();
   void createFeet();
   void setMaskBitsCrouching(bool enabled);

   float getMaxVelocity() const;
   float readVelocityFromController(const PlayerSpeed& speed) const;
   float readVelocityFromKeyboard(const PlayerSpeed& speed) const;
   float readDesiredVelocity(const PlayerSpeed& speed) const;
   float readDesiredVelocity() const;
   float getDeceleration() const;
   float getAcceleration() const;

   void traceJumpCurve();
   void keyPressed(sf::Keyboard::Key key);
   std::unique_ptr<ScreenTransition> makeFadeTransition();

   void drawDash(sf::RenderTarget& color, const std::shared_ptr<Animation>& current_cycle, const sf::Vector2f& draw_position_px);
   bool checkDamageDrawSkip() const;
   void updateHurtColor(const std::shared_ptr<Animation>& current_cycle);

   void useInventory(int32_t slot);

   ToggleCallback _toggle_callback;
   std::shared_ptr<WeaponSystem> _weapon_system;

   // all related to player physics and box2d
   std::shared_ptr<b2World> _world;
   b2Body* _body = nullptr;
   static constexpr int32_t __foot_count = 4u;
   b2Fixture* _body_fixture = nullptr;
   b2Fixture* _foot_fixture[__foot_count]{nullptr, nullptr, nullptr, nullptr};
   b2Fixture* _foot_sensor_fixture = nullptr;
   b2Body* _platform_body = nullptr;
   float _platform_dx{0.0f};
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
   sf::Clock _portal_clock;
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
   PlayerBend _bend;
   PlayerClimb _climb;
   PlayerJump _jump;
   PlayerDash _dash;
   PlayerAttack _attack;
   PlayerBelt _belt;
   PlayerJumpTrace _jump_trace;
   WaterBubbles _water_bubbles;

   PlayerAnimation _player_animation;
   std::deque<PositionedAnimation> _last_animations;

   Chunk _chunk;
   AnimationPool _animation_pool{"data/sprites/animations.json"};

   static Player* __current;
};
