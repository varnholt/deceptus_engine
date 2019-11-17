#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <variant>

// box2d
#include "Box2D/Box2D.h"

// sfml
#include "SFML/Graphics.hpp"

// game
#include "leveldescription.h"
#include "gamenode.h"
#include "weapon.h"

struct lua_State;

struct LuaNode : public GameNode
{
   LuaNode(const std::string &filename);
   ~LuaNode();

   void draw(sf::RenderTarget& window);
   void initialize();

   void setupLua();
   void setupTexture();

   void addShapeCircle(float radius, float x, float y);
   void addShapePoly(const b2Vec2* points, int32_t size);
   void addShapeRect(float width, float height, float x, float y);
   void addWeapon(std::unique_ptr<b2Shape> shape, int32_t fireInterval);
   void boom(float x, float y, float intensity);
   void damage(int32_t damage, float forceX, float forceY);
   void damageRadius(int32_t damage, float x, float y, float radius);
   b2Vec2 getLinearVelocity() const;
   void fireWeapon(size_t index, b2Vec2 from, b2Vec2 to);
   void makeDynamic();
   void makeStatic();
   void setActive(bool active);
   void setDamage(int32 damage);
   void setGravityScale(float scale);
   void setTransform(const b2Vec2& position, float32 angle = 0.0);
   void updatePosition();
   void updateSpriteRect(int32_t x, int32_t y, int32_t w, int32_t h);
   void updateVelocity();
   const EnemyDescription& getEnemyDescription() const;
   void setEnemyDescription(const EnemyDescription& enemyDescription);

   void luaDie();
   void luaInitialize();
   void luaMovedTo();
   void luaSetStartPosition();
   void luaPlayerMovedTo();
   void luaRetrieveProperties();
   void luaSendPath(const std::vector<sf::Vector2f> &vec);
   void luaSendPatrolPath();
   void luaTimeout(int32_t timerId);
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaCollisionWithPlayer();

   // property accessors
   void synchronizeProperties();
   bool getPropertyBool(const std::string& key);
   double getPropertyDouble(const std::string& key);
   int64_t getPropertyInt64(const std::string& key);

   // box2d related
   void createBody();
   void stopScript();

   // members
   int32_t mId = -1;
   int32_t mKeysPressed = 0;
   std::string mScriptName;
   lua_State* mState = nullptr;
   EnemyDescription mEnemyDescription;

   // visualization
   sf::Vector2f mStartPosition;
   sf::Texture mTexture;
   sf::Sprite mSprite;
   sf::Vector2u mSpriteOffset;
   int32_t mSpriteWidth = 0;
   int32_t mSpriteHeight = 0;
   sf::Vector2f mPosition;
   std::vector<sf::Vector2f> mPatrolPath;

   // physics
   b2Body* mBody = nullptr;
   b2BodyDef* mBodyDef = nullptr;
   std::vector<b2Shape*> mShapes;
   std::vector<std::unique_ptr<Weapon>> mWeapons;

   std::map<std::string, std::variant<std::string, int64_t, double, bool>> mProperties;

   // static
   static std::atomic<int32_t> sNextId;

   public:
   void deserializeEnemyDescription();
};

