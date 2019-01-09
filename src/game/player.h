#ifndef PLAYER_H
#define PLAYER_H

#include "constants.h"
#include "extramanager.h"
#include "extratable.h"
#include "gamenode.h"
#include "joystick/gamecontrollerinfo.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "Box2D/Box2D.h"
#include <functional>
#include <memory>
#include <set>

class GameContactListener;
class Weapon;

const sf::Vector2f vector2fZero;

class Player : public GameNode
{

   class PlayerAABBQueryCallback : public b2QueryCallback
   {
      public:
         std::set<b2Body*> mBodies;

      public:
         bool ReportFixture(b2Fixture* fixture)
         {
            // foundBodies.push_back(fixture->GetBody());
            mBodies.insert(fixture->GetBody());

            // keep going to find all fixtures in the query area
            return true;
         }
   };

   enum class Edge
   {
      None,
      Left,
      Right
   };

   enum class Dash
   {
     None,
     Left,
     Right
   };


public:

   std::shared_ptr<ExtraTable> mExtraTable;


protected:

   b2World* mWorld = nullptr;
   b2Body* mBody = nullptr;
   b2Fixture* mHeadFixture = nullptr;
   b2Joint* mClimbJoint = nullptr;

   Weapon* mWeapon = nullptr;
   std::shared_ptr<ExtraManager> mExtraManager;

   sf::Vector2f mPixelPosition;

   GameControllerInfo mJoystickInfo;
   int mKeysPressed = 0;
   bool mControllerRunPressed = false;

   sf::Texture mTexture;
   sf::Sprite mSprite;
   sf::Vector2u mSpritePrev;
   sf::Vector2u mSpriteAnim;
   sf::Clock mClock;
   sf::Time mGroundContactLostTime;
   sf::Time mLastJumpPressTime;

   int mAnimSpeed = 50;

   sf::Clock mJumpClock;
   int mJumpSteps = 0;

   bool mPointsToLeft = false;
   bool mVisible = true;

   float mPlatformVelocity = 0.0f;
   bool mInWater = false;
   float mTime = 0.0f;
   float mNextFootStepTime = 0.0f;
   int mZ = 0;
   int mId = 0;
   bool mHadGroundContact = true;
   bool mGroundContactJustLost = false;
   float mBeltVelocity = 0.0f;
   bool mIsOnBelt = false;

   int mDashSteps = 20;
   Dash mDashDir = Dash::None;

   sf::Clock mPortalClock;
   sf::Clock mDamageClock;

   static int sNextId;
   static std::vector<Player*> sPlayerList;


public:

   template <typename T>
   T minimum(T a, T b)
   {
      return a < b ? a : b;
   }

   template <typename T>
   T maximum(T a, T b)
   {
      return a > b ? a : b;
   }

   Player(GameNode* parent = nullptr);
   virtual ~Player();


   void initialize();
   void initializeController();
   void draw(sf::RenderTarget& target);

   void update(float dt);
   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);

   void controllerRunButtonPressed();
   void controllerRunButtonReleased();
   bool isLookingAround() const;
   bool isControllerUsed() const;
   bool isControllerButtonPressed(int buttonEnum) const;
   bool isFireButtonPressed() const;
   bool isJumpButtonPressed() const;
   bool isJumping() const;

   void jump();
   void fire();
   void die();
   void reset();
   bool isDead() const;

   bool isMovingRight() const;
   bool isMovingLeft() const;
   bool isMoving() const;

   void setStartPixelPosition(float x, float y);

   int getKeysPressed() const;
   void setKeysPressed(int keys);

   b2Vec2 getBodyPosition() const;
   sf::Vector2f getPixelPosition() const;
   void setPixelPosition(float x, float y);

   float getBeltVelocity() const;
   void setBeltVelocity(float beltVelocity);
   bool isOnBelt() const;
   void setOnBelt(bool onBelt);
   void applyBeltVelocity(float &desiredVel);

   sf::Rect<int> getPlayerRect() const;


   b2Body* getBody() const;
   b2World* getWorld() const;
   void setWorld(b2World *world);
   void updatePixelPosition();
   void setBodyViaPixelPosition(float x, float y);
   void setFriction(float f);


   bool getVisible() const;
   void setVisible(bool visible);

   static Player* getPlayer(int id);


   float getPlatformVelocity() const;
   void setPlatformVelocity(float platformVelocity);

   bool isInAir() const;
   bool isClimbing() const;
   bool isInWater() const;

   void setInWater(bool inWater);

   int getZ() const;
   void setZ(int z);

   int getId() const;

   const GameControllerInfo& getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo &joystickInfo);

   void damage(int damage, const sf::Vector2f& force = vector2fZero);

   void updateClimb();

   std::shared_ptr<ExtraManager> getExtraManager() const;


private:

   void createPlayerBody();

   void jumpImpulse();
   void jumpForce();

   void updateDash(Dash dir = Dash::None);

   void updateExtraManager();
   void updateAtmosphere();
   void updateVelocity();
   void updateJumpBuffer();
   void updateJump();
   void updatePlatformMovement(float dt);
   void updateFire();
   void updateFootsteps();
   void updatePortal();
   void updateLostGroundContact();
   void updatePlayerOrientation();

   void createHead();
   void createFeet();
   void setHeadEnabled(bool enabled);
   void createBody();

   bool isClimbableEdge(b2ChainShape* shape, int currIndex);
   void removeClimbJoint();
   bool edgeMatchesMovement(const b2Vec2 &edgeDir);
   float getMaxVelocity() const;
   float getVelocityFromController(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const;
   float getVelocityFromKeyboard(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const;
   float getDesiredVelocity(float velocityMax, const b2Vec2& velocity, float slowdown, float acceleration) const;
   float getDesiredVelocity() const;
   float getSlowDown() const;
   float getAcceleration() const;
   void updateAnimationOffset();
};

#endif // PLAYER_H
