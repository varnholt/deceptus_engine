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
   void deserializeEnemyDescription();

   void setupLua();
   void setupTexture();
   void updatePosition();
   void updateVelocity();
   void updateWeapons(const sf::Time& dt);

   //! add a circle shape to the body of the object
   void addShapeCircle(float radius, float x, float y);

   //! add a polygon shape to the body of the object
   void addShapePoly(const b2Vec2* points, int32_t size);

   //! add a rectangular shape to the body of the object (given in metres)
   void addShapeRect(float width, float height, float center_x, float center_y);

   //! add a weapon to the node
   void addWeapon(std::unique_ptr<Weapon> weapon);

   //! trigger boom effect
   void boom(float x, float y, float intensity);

   //! player detonation animation
   void playDetonationAnimation(float x, float y);

   //! cause damage to the player
   void damage(int32_t damage, float forceX, float forceY);

   //! cause damage within a given radius
   void damageRadius(int32_t damage, float x, float y, float radius);

   //! get the body's linear velocity
   b2Vec2 getLinearVelocity() const;

   //! set the body's linear velocity
   void setLinearVelocity(const b2Vec2& vel);

   //! fire a weapon
   void fireWeapon(size_t index, b2Vec2 from, b2Vec2 to);

   //! make the body a dynamic object
   void makeDynamic();

   //! make the body a static object
   void makeStatic();

   //! query bodies within a given aabb
   int32_t queryAABB(const b2AABB& aabb);

   //! cast a ray between two points
   int32_t queryRaycast(const b2Vec2& point1, const b2Vec2& point2);

   //! activate or deactivate a body
   void setActive(bool active);

   //! set the node's damage for collisions with the player
   void setDamage(int32_t damage);

   //! set the object's gravity scale
   void setGravityScale(float scale);

   //! set the object's transform
   void setTransform(const b2Vec2& position, float32 angle = 0.0);

   //! add a sprite
   void addSprite();

   //! set the sprite's origin
   void setSpriteOrigin(int32_t id, float x, float y);

   //! set the sprite's position
   void setSpriteOffset(int32_t id, float x, float y);

   //! update the sprite's texture rect
   void updateSpriteRect(int32_t id, int32_t x, int32_t y, int32_t w, int32_t h);


   void luaHit(int32_t damage);
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
   void setupBody();
   void stopScript();

   // members
   int32_t _id = -1;
   int32_t _keys_pressed = 0;
   std::string _script_name;
   lua_State* _lua_state = nullptr;
   EnemyDescription _enemy_description;

   // visualization
   sf::Vector2f _start_position;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _sprites = {{}};              // have 1 base sprite
   std::vector<sf::Vector2f> _sprite_offsets = {{0, 0}};   // have 1 base sprite offset
   sf::Vector2f _position;
   int32_t _z_index = static_cast<int32_t>(ZDepth::Player);
   std::vector<sf::Vector2f> _movement_path;

   // physics
   b2Body* _body = nullptr;
   b2BodyDef* _body_def = nullptr;
   std::vector<b2Shape*> _shapes;
   std::vector<std::unique_ptr<Weapon>> _weapons;

   std::map<std::string, std::variant<std::string, int64_t, double, bool>> _properties;

   // static
   static std::atomic<int32_t> __next_id;
};

