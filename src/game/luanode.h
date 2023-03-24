#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>

// box2d
#include "Box2D/Box2D.h"

// sfml
#include "SFML/Graphics.hpp"

// game
#include "detonationanimation.h"
#include "gamemechanism.h"
#include "gamenode.h"
#include "hitbox.h"
#include "leveldescription.h"
#include "weapon.h"

struct lua_State;

/*! \brief LuaNode is the class that implements scripted enemies.
 *         Enemy behavior is implemented in lua scripts, one instance is created for each enemy in the game.
 *
 * The LuaNode behavior is driven from two directions; the scripts can call each callback registered in setupLua.
 * From the C++ end, the scripts can be driven by calling lua* functions such as luaHit, luaDie, etc.
 */
struct LuaNode : public GameMechanism, public GameNode
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   LuaNode(GameNode* parent, const std::string& filename);
   ~LuaNode();

   void draw(sf::RenderTarget& window, sf::RenderTarget& normal) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void initialize();
   void deserializeEnemyDescription();

   void setupLua();
   void setupTexture();
   void updatePosition();
   void updateVelocity();
   void updateWeapons(const sf::Time& dt);
   void updateHitboxOffsets();

   //! add a circle shape to the body of the object
   void addShapeCircle(float radius, float x, float y);

   //! add a polygon shape to the body of the object
   void addShapePoly(const b2Vec2* points, int32_t size);

   //! add a rectangular shape to the body of the object (given in metres)
   void addShapeRect(float width, float height, float center_x, float center_y);

   //! add a beveled rect
   void addShapeRectBevel(float width, float height, float bevel, float offset_x = 0.0f, float offset_y = 0.0f);

   //! add a weapon to the node
   void addWeapon(std::unique_ptr<Weapon> weapon);

   //! trigger boom effect
   void boom(float x, float y, float intensity);

   //! player huge detonation animation
   void playDetonationAnimationHuge(float x, float y);

   //! player detonation animation
   void playDetonationAnimation(const std::vector<DetonationAnimation::DetonationRing>& rings);

   //! cause damage to the player
   void damagePlayer(int32_t damage, float forceX, float forceY);

   //! cause damage within a given radius
   void damagePlayerInRadius(int32_t damage, float x, float y, float radius);

   //! get the body's linear velocity
   b2Vec2 getLinearVelocity() const;

   //! set the body's linear velocity
   void setLinearVelocity(const b2Vec2& vel);

   //! apply linear impulse to body
   void applyLinearImpulse(const b2Vec2& vel);

   //! apply a force to the body
   void applyForce(const b2Vec2& force);

   //! fire a gun
   void useGun(size_t index, b2Vec2 from, b2Vec2 to);

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
   void setDamageToPlayer(int32_t damage);

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

   //! set a sprite's color
   void setSpriteColor(int32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

   //! update a debug rect
   void updateDebugRect(int32_t index, float left_px, float top_px, float width_px, float height_px);

   //! add a debug rect
   void addDebugRect();

   //! add a hitbox
   void addHitbox(int32_t left_px, int32_t top_px, int32_t width_px, int32_t height_px);

   //! add an audio range
   void addAudioRange(float far_distance, float far_volume, float near_distance, float near_volume);

   //! add a sample
   void addSample(const std::string& sample);

   //! play a sample
   void playSample(const std::string& sample, float volume);

   //! node is dead, reset its body, set dead flag
   void die();

   const std::optional<HighResTimePoint> getHitTime() const;

   int32_t getDamageFromPlayer() const;

   // all functions that 'speak' directly to the lua scripts
   void luaHit(int32_t damage);
   void luaInitialize();
   void luaMovedTo();
   void luaSetStartPosition();
   void luaPlayerMovedTo();
   void luaRetrieveProperties();
   void luaSendPath(const std::vector<sf::Vector2f>& vec);
   void luaSendPatrolPath();
   void luaTimeout(int32_t timerId);
   void luaUpdate(const sf::Time& dt);
   void luaWriteProperty(const std::string& key, const std::string& value);
   void luaCollisionWithPlayer();

   // property accessors
   void synchronizeProperties();
   bool getPropertyBool(const std::string& key, bool default_value = false);
   double getPropertyDouble(const std::string& key, double default_value = 0.0);
   int64_t getPropertyInt64(const std::string& key, int64_t default_value = 0);

   // box2d related
   void setupBody();
   void stopScript();

   // members
   int32_t _id{-1};
   int32_t _keys_pressed{0};
   std::string _script_name;
   lua_State* _lua_state{nullptr};
   EnemyDescription _enemy_description;

   // visualization
   sf::Vector2f _start_position_px;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _sprites = {{}};                  // have 1 base sprite
   std::vector<sf::Vector2f> _sprite_offsets_px = {{0, 0}};  // have 1 base sprite offset
   sf::Vector2f _position_px;
   std::vector<sf::Vector2f> _movement_path_px;
   sf::Shader _flash_shader;
   float _hit_flash{0.0f};
   std::vector<sf::FloatRect> _debug_rects;

   // physics
   b2Body* _body{nullptr};
   b2BodyDef* _body_def{nullptr};
   std::vector<b2Shape*> _shapes_m;
   std::vector<std::unique_ptr<Weapon>> _weapons;

   // damage
   std::vector<Hitbox> _hitboxes;
   std::optional<sf::FloatRect> _bounding_box;
   std::optional<HighResTimePoint> _hit_time;
   int32_t _damage_from_player{0};
   bool _dead{false};

   std::map<std::string, std::variant<std::string, int64_t, double, bool>> _properties;
};
