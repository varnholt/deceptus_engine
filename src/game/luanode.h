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
#include "gamenode.h"
#include "weapon.h"

struct lua_State;

struct LuaNode : public GameNode
{
   LuaNode(const std::string &filename);

   void initialize();

   void setupLua();
   void setupTexture();

   void synchronizeProperties();
   void updateVelocity();
   void updatePosition();
   void updateSpriteRect(int x, int y, int w, int h);
   void addShapeCircle(float radius, float x, float y);
   void addShapeRect(float width, float height, float x, float y);
   void addShapePoly(const b2Vec2* points, int32_t size);
   void addWeapon(std::unique_ptr<b2Shape> shape, int fireInterval);

   void draw(sf::RenderTarget& window);

   void luaInitialize();
   void luaAct(float dt);
   void luaMovedTo();
   void luaPlayerMovedTo();
   void luaDied();
   void luaRetrieveProperties();
   void luaTimeout(int timerId);
   void luaSendPatrolPath();
   void luaSendPath(const std::vector<sf::Vector2f> &vec);
   void damage(int playerId, int damage, float forceX, float forceY);

   // property accessors
   bool getPropertyBool(const std::string& key);
   double getPropertyDouble(const std::string& key);
   int64_t getPropertyInt64(const std::string& key);


   // box2d related
   void createBody();
   void stopScript();

   // members
   int mId = -1;
   int mKeysPressed = 0;
   std::string mScriptName;
   lua_State* mState = nullptr;

   sf::Vector2f mStartPosition;
   sf::Texture mTexture;
   sf::Sprite mSprite;
   sf::Vector2u mSpriteOffset;
   int mSpriteWidth = 0;
   int mSpriteHeight = 0;
   sf::Vector2f mPosition;
   std::vector<sf::Vector2f> mPatrolPath;

   std::shared_ptr<b2Body> mBody;
   std::shared_ptr<b2BodyDef> mBodyDef;
   std::vector<std::shared_ptr<b2Shape>> mShapes;
   std::vector<std::unique_ptr<Weapon>> mWeapons;

   std::map<std::string, std::variant<std::string, int64_t, double, bool>> mProperties;

   // static
   static std::atomic<int> sNextId;
};

