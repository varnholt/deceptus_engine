#pragma once

#include "constants.h"
#include "extramanager.h"
#include "extratable.h"
#include "gamenode.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "playeranimation.h"
#include "playerclimb.h"
#include "playercontrols.h"
#include "playerjump.h"
#include "shaders/deathshader.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "Box2D/Box2D.h"

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <set>

class Animation;
class GameContactListener;
class Weapon;
struct WeaponSystem;

class Player : public GameNode
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   struct JumpTrace
   {
      bool jumpStarted = false;
      sf::Time jumpStartTime;
      float jumpStartY = 0.0f;
      float jumpEpsilon = 0.00001f;
      float jumpPrevY = 0.0f;
   };


   struct PositionedAnimation
   {
      sf::Vector2f mPosition;
      std::shared_ptr<Animation> mAnimation;
   };


   struct PlayerSpeed
   {
      b2Vec2 currentVelocity;
      float velocityMax = 0.0f;
      float acceleration = 0.0f;
      float deceleration = 0.0f;
   };

   struct PlayerDash
   {
      int32_t _dash_frame_count = 0;
      float _dash_multiplier = 0.0f;
      Dash _dash_dir = Dash::None;

      bool isDashActive() const
      {
         return (_dash_frame_count > 0);
      }
   };

   struct PlayerBend
   {
      bool _bending_down = false;
      bool _was_bending_down = false;
      bool _crouching = false;
      bool _was_crouching = false;
      HighResTimePoint _timepoint_bend_down_start;
      HighResTimePoint _timepoint_bend_down_end;

      bool isCrouching() const
      {
         return _bending_down;
      }
   };


public:

   Player(GameNode* parent = nullptr);
   virtual ~Player() = default;

   static Player* getCurrent();

   void initialize();
   void initializeLevel();
   void initializeController();
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal);

   void update(const sf::Time& dt);

   void fire();
   void die();
   void reset();
   DeathReason checkDead() const;

   bool isPointingRight() const;
   bool isPointingLeft() const;

   void setStartPixelPosition(float x, float y);

   b2Vec2 getBodyPosition() const;
   const sf::Vector2f& getPixelPositionf() const;
   const sf::Vector2i& getPixelPositioni() const;
   void setPixelPosition(float x, float y);

   float getBeltVelocity() const;
   void setBeltVelocity(float beltVelocity);
   bool isOnBelt() const;
   void setOnBelt(bool onBelt);
   void applyBeltVelocity(float &desiredVel);

   const sf::IntRect& getPlayerPixelRect() const;


   b2Body* getBody() const;
   void setWorld(const std::shared_ptr<b2World>& world);
   void resetWorld();
   void updatePixelPosition();
   void updatePreviousBodyState();
   void updatePlayerPixelRect();
   void setBodyViaPixelPosition(float x, float y);
   void setFriction(float f);


   bool getVisible() const;
   void setVisible(bool visible);


   b2Body* getPlatformBody() const;
   void setPlatformBody(b2Body* body);
   void setGroundBody(b2Body* body);

   bool isInAir() const;
   bool isInWater() const;
   bool isOnPlatform() const;
   bool isOnGround() const;
   bool isDead() const;

   void setInWater(bool inWater);

   int getZIndex() const;
   void setZIndex(int32_t z);

   int getId() const;

   void impulse(float intensity);
   void damage(int32_t damage, const sf::Vector2f& force = sf::Vector2f{0.0f, 0.0f});

   std::shared_ptr<ExtraManager> getExtraManager() const;

   PlayerControls& getControls();

   std::shared_ptr<WeaponSystem> getWeaponSystem() const;

   PlayerAnimation& getPlayerAnimation();


private:

   void createPlayerBody();

   void updateAnimation(const sf::Time& dt);
   void updatePixelCollisions();
   void updateAtmosphere();
   void updateVelocity();
   void updatePlatformMovement(const sf::Time& dt);
   void updateFire();
   void updateFootsteps();
   void updatePortal();
   void updatePlayerOrientation();
   void updateDash(Dash dir = Dash::None);
   void updateBendDown();
   void updateHardLanding();
   void updateGroundAngle();
   void updateDeadFixtures();
   void updateWeapons(const sf::Time& dt);
   void updateImpulse();

   void resetDash();

   void createBody();
   void createFeet();
   void setMaskBitsCrouching(bool enabled);

   float getMaxVelocity() const;
   float getVelocityFromController(const PlayerSpeed& speed) const;
   float getVelocityFromKeyboard(const PlayerSpeed& speed) const;
   float getDesiredVelocity(const PlayerSpeed& speed) const;
   float getDesiredVelocity() const;
   float getDeceleration() const;
   float getAcceleration() const;

   void playDustAnimation();
   void traceJumpCurve();
   void keyPressed(sf::Keyboard::Key key);

   std::shared_ptr<WeaponSystem> _weapon_system;
   std::shared_ptr<ExtraManager> _extra_manager;

   // all related to player physics and box2d
   std::shared_ptr<b2World> _world;
   b2Body* _body = nullptr;
   static constexpr int32_t __foot_count = 4u;
   b2Fixture* _body_fixture = nullptr;
   b2Fixture* _foot_fixture[__foot_count];
   b2Body* _platform_body = nullptr;
   b2Body* _ground_body = nullptr;
   b2Vec2 _ground_normal;
   b2Vec2 _position_previous;
   b2Vec2 _velocity_previous;
   float _impulse = 0.0f;

   sf::Vector2f _pixel_position_f;
   sf::Vector2i _pixel_position_i;
   sf::IntRect _pixel_rect;

   sf::Time _time;
   sf::Clock _clock;
   sf::Clock _portal_clock;
   sf::Clock _damage_clock;
   bool _damage_initialized = false;

   bool _points_to_left = false;
   bool _visible = true;
   bool _in_water = false;
   bool _dead = false;

   float _next_footstep_time = 0.0f;

   int _z_index = 0;
   int _id = 0;

   bool _hard_landing = false;
   int32_t _hard_landing_cycles = 0;
   HighResTimePoint _timepoint_hard_landing;

   float _belt_velocity = 0.0f;
   bool _is_on_belt = false;

   PlayerControls _controls;
   PlayerBend _bend;
   PlayerClimb _climb;
   PlayerJump _jump;
   PlayerDash _dash;
   JumpTrace _jump_trace;

   PlayerAnimation _player_animation;
   std::deque<PositionedAnimation> _last_animations;

   static Player* __current;
};

