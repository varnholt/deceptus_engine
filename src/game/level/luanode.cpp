// base
#include "luanode.h"

#include <lua.hpp>

// box2d
#include "box2d/box2d.h"

// game
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxparser.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tools/log.h"
#include "framework/tools/timer.h"
#include "game/animation/animationplayer.h"
#include "game/animation/detonationanimation.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/level/level.h"
#include "game/level/luaconstants.h"
#include "game/level/luainterface.h"
#include "game/level/luanodecallbacks.h"
#include "game/physics/physicsconfiguration.h"
#include "game/player/player.h"
#include "game/state/savestate.h"
#include "game/weapons/gun.h"
#include "game/weapons/projectilehitaudio.h"
#include "game/weapons/weapon.h"
#include "game/weapons/weaponfactory.h"

namespace
{
uint16_t category_bits_default = CategoryEnemyWalkThrough;         // I am a ...
uint16_t mask_bits_default = CategoryBoundary | CategoryFriendly;  // I collide with ...
int16_t group_index_default = 0;                                   // 0 is default
}  // namespace

void LuaNode::setupTexture()
{
   // assume the texture is only configured once
   if (_texture)
   {
      return;
   }

   const auto sprite_name = std::get<std::string>(_properties["sprite"]);
   _texture = TexturePool::getInstance().get(sprite_name);
   addSprite();
}

LuaNode::LuaNode(GameNode* parent, const std::string& filename) : GameNode(parent), _script_name(filename)
{
   _z_index = static_cast<int32_t>(ZDepth::Player);

   setClassName(typeid(LuaNode).name());

   // create instances
   _body_def = new b2BodyDef();
   _body = Level::getCurrentLevel()->getWorld()->CreateBody(_body_def);
}

LuaNode::~LuaNode()
{
   // Log::Info() << "stopping script: " << _script_name << " address: " << this;
   stopScript();
}

std::string_view LuaNode::objectName() const
{
   return "LuaNode";
}

void LuaNode::deserializeEnemyDescription()
{
   _object_id = _enemy_description._id;
   _name = _enemy_description._name;

   // set up patrol path
   if (!_enemy_description._path.empty())
   {
      std::vector<sf::Vector2f> patrol_path;

      for (auto i = 0u; i < _enemy_description._path.size(); i += 2)
      {
         auto pos =
            sf::Vector2f(static_cast<float_t>(_enemy_description._path.at(i)), static_cast<float_t>(_enemy_description._path.at(i + 1)));

         // by default the path is given is tiles.
         // if we override it, we're setting pixel positions which are already transformed
         if (_enemy_description._position_in_tiles)
         {
            pos.x *= PIXELS_PER_TILE;
            pos.y *= PIXELS_PER_TILE;
            pos.x += PIXELS_PER_TILE / 2;
            pos.y += PIXELS_PER_TILE / 2;
         }

         patrol_path.push_back(pos);
      }

      _movement_path_px = patrol_path;
   }

   // set up start position
   if (!_enemy_description._start_position.empty())
   {
      _start_position_px = sf::Vector2f(
         static_cast<float_t>(_enemy_description._start_position.at(0)), static_cast<float_t>(_enemy_description._start_position.at(1))
      );

      if (_enemy_description._position_in_tiles)
      {
         _start_position_px.x *= PIXELS_PER_TILE;
         _start_position_px.y *= PIXELS_PER_TILE;
         _start_position_px.x += PIXELS_PER_TILE / 2;
         _start_position_px.y += PIXELS_PER_TILE / 2;
      }

      _position_px = _start_position_px;
   }
}

void LuaNode::initialize()
{
   deserializeEnemyDescription();
   setupLua();
   setupBody();

   if (!_flash_shader.loadFromFile("data/shaders/flash.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading flash shader";
   }

   _flash_shader.setUniform("texture", sf::Shader::CurrentTexture);
   _flash_shader.setUniform("flash", _hit_flash);
}

void LuaNode::setupLua()
{
   _lua_state = luaL_newstate();

   // register callbacks
   lua_register(_lua_state, "addAudioRange", LuaNodeCallbacks::addAudioRange);
   lua_register(_lua_state, "addDebugRect", LuaNodeCallbacks::addDebugRect);
   lua_register(_lua_state, "addHitbox", LuaNodeCallbacks::addHitbox);
   lua_register(_lua_state, "addPlayerSkill", LuaNodeCallbacks::addPlayerSkill);
   lua_register(_lua_state, "addSample", LuaNodeCallbacks::addSample);
   lua_register(_lua_state, "addShapeCircle", LuaNodeCallbacks::addShapeCircle);
   lua_register(_lua_state, "addShapeRect", LuaNodeCallbacks::addShapeRect);
   lua_register(_lua_state, "addShapeRectBevel", LuaNodeCallbacks::addShapeRectBevel);
   lua_register(_lua_state, "addShapePoly", LuaNodeCallbacks::addShapePoly);
   lua_register(_lua_state, "addSprite", LuaNodeCallbacks::addSprite);
   lua_register(_lua_state, "addWeapon", LuaNodeCallbacks::addWeapon);
   lua_register(_lua_state, "applyForce", LuaNodeCallbacks::applyForce);
   lua_register(_lua_state, "applyLinearImpulse", LuaNodeCallbacks::applyLinearImpulse);
   lua_register(_lua_state, "boom", LuaNodeCallbacks::boom);
   lua_register(_lua_state, "damage", LuaNodeCallbacks::damage);
   lua_register(_lua_state, "damageRadius", LuaNodeCallbacks::damageRadius);
   lua_register(_lua_state, "die", LuaNodeCallbacks::die);
   lua_register(_lua_state, "getLinearVelocity", LuaNodeCallbacks::getLinearVelocity);
   lua_register(_lua_state, "getGravity", LuaNodeCallbacks::getGravity);
   lua_register(_lua_state, "intersectsWithPlayer", LuaNodeCallbacks::intersectsWithPlayer);
   lua_register(_lua_state, "isPhsyicsPathClear", LuaNodeCallbacks::isPhsyicsPathClear);
   lua_register(_lua_state, "isPlayerDead", LuaNodeCallbacks::isPlayerDead);
   lua_register(_lua_state, "log", LuaNodeCallbacks::debug);
   lua_register(_lua_state, "makeDynamic", LuaNodeCallbacks::makeDynamic);
   lua_register(_lua_state, "makeStatic", LuaNodeCallbacks::makeStatic);
   lua_register(_lua_state, "playDetonationAnimation", LuaNodeCallbacks::playDetonationAnimation);
   lua_register(_lua_state, "playSample", LuaNodeCallbacks::playSample);
   lua_register(_lua_state, "queryAABB", LuaNodeCallbacks::queryAABB);
   lua_register(_lua_state, "queryRayCast", LuaNodeCallbacks::queryRayCast);
   lua_register(_lua_state, "registerHitAnimation", LuaNodeCallbacks::registerHitAnimation);
   lua_register(_lua_state, "registerHitSamples", LuaNodeCallbacks::registerHitSamples);
   lua_register(_lua_state, "removePlayerSkill", LuaNodeCallbacks::removePlayerSkill);
   lua_register(_lua_state, "setActive", LuaNodeCallbacks::setActive);
   lua_register(_lua_state, "setAudioUpdateBehavior", LuaNodeCallbacks::setAudioUpdateBehavior);
   lua_register(_lua_state, "setDamage", LuaNodeCallbacks::setDamageToPlayer);
   lua_register(_lua_state, "setGravityScale", LuaNodeCallbacks::setGravityScale);
   lua_register(_lua_state, "setLinearVelocity", LuaNodeCallbacks::setLinearVelocity);
   lua_register(_lua_state, "setReferenceVolume", LuaNodeCallbacks::setReferenceVolume);
   lua_register(_lua_state, "setSpriteColor", LuaNodeCallbacks::setSpriteColor);
   lua_register(_lua_state, "setSpriteOffset", LuaNodeCallbacks::setSpriteOffset);
   lua_register(_lua_state, "setSpriteOrigin", LuaNodeCallbacks::setSpriteOrigin);
   lua_register(_lua_state, "setSpriteScale", LuaNodeCallbacks::setSpriteScale);
   lua_register(_lua_state, "setSpriteVisible", LuaNodeCallbacks::setSpriteVisible);
   lua_register(_lua_state, "setTransform", LuaNodeCallbacks::setTransform);
   lua_register(_lua_state, "setVisible", LuaNodeCallbacks::setVisible);
   lua_register(_lua_state, "setZ", LuaNodeCallbacks::setZIndex);
   lua_register(_lua_state, "timer", LuaNodeCallbacks::timer);
   lua_register(_lua_state, "updateDebugRect", LuaNodeCallbacks::updateDebugRect);
   lua_register(_lua_state, "updateKeysPressed", LuaNodeCallbacks::updateKeysPressed);
   lua_register(_lua_state, "updateProjectileAnimation", LuaNodeCallbacks::updateProjectileAnimation);
   lua_register(_lua_state, "updateProjectileTexture", LuaNodeCallbacks::updateProjectileTexture);
   lua_register(_lua_state, "updateProperties", LuaNodeCallbacks::updateProperties);
   lua_register(_lua_state, "updateSpriteRect", LuaNodeCallbacks::updateSpriteRect);
   lua_register(_lua_state, "useWeapon", LuaNodeCallbacks::useWeapon);

   // make standard libraries available in the Lua object
   luaL_openlibs(_lua_state);

   // load program
   auto result = luaL_loadfile(_lua_state, _script_name.c_str());
   if (result == LUA_OK)
   {
      // execute program
      result = lua_pcall(_lua_state, 0, LUA_MULTRET, 0);

      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state);
      }
      else
      {
         luaSetStartPosition();
         luaMovedTo();
         luaRetrieveProperties();
         luaInitialize();
         luaSendPatrolPath();
      }
   }
   else
   {
      LuaNodeCallbacks::error(_lua_state);
   }

   // register properties
   for (const auto& prop : _enemy_description._properties)
   {
      luaWriteProperty(prop._name, prop._value);
   }
}

void LuaNode::synchronizeProperties()
{
   // evaluate property map
   //
   //   int32_t i = std::get<int>(variant);
   //   w = std::get<int>(variant);
   //   w = std::get<0>(variant);

   // as soon as the texture is known, it can be set up
   setupTexture();
}

/**
 * @brief LuaNode::luaInitialize called to call the initialize function inside the lua script
 * callback name: initialize
 */
void LuaNode::luaInitialize()
{
   lua_getglobal(_lua_state, FUNCTION_INITIALIZE);
   const auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_INITIALIZE);
   }
}

/**
 * @brief LuaNode::luaUpdate update the lua node
 * @param dt delta time, passed to luanode in seconds
 * callback name: update
 */
void LuaNode::luaUpdate(const sf::Time& dt)
{
   lua_getglobal(_lua_state, FUNCTION_UPDATE);
   lua_pushnumber(_lua_state, dt.asSeconds());

   const auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_UPDATE);
   }
}

/**
 * @brief LuaNode::luaWriteProperty write a property of the luanode
 * @param key property key
 * @param value property value
 * callback name: writeProperty
 */
void LuaNode::luaWriteProperty(const std::string& key, const std::string& value)
{
   lua_getglobal(_lua_state, FUNCTION_WRITE_PROPERTY);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushstring(_lua_state, key.c_str());
      lua_pushstring(_lua_state, value.c_str());

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_WRITE_PROPERTY);
      }
   }
}

/**
 * @brief LuaNode::luaHit luanode got hit by something
 * @param damage amount of damage from 0..100 while 100 is fatal
 * callback name: hit
 */
void LuaNode::luaHit(int32_t damage)
{
   // Log::Info() << "thing was hit: " << damage;
   _hit_time = std::chrono::high_resolution_clock::now();
   _damage_from_player = damage;

   lua_getglobal(_lua_state, FUNCTION_HIT);
   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushinteger(_lua_state, damage);

      const auto result = lua_pcall(_lua_state, 1, 0, 0);
      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_HIT);
      }
   }
}

/**
 * @brief LuaNode::luaCollisionWithPlayer indicate collision with player
 * callback name: collisionWithPlayer
 */
void LuaNode::luaCollisionWithPlayer()
{
   lua_getglobal(_lua_state, FUNCTION_COLLISION_WITH_PLAYER);
   if (lua_isfunction(_lua_state, -1))
   {
      const auto result = lua_pcall(_lua_state, 0, 0, 0);
      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_COLLISION_WITH_PLAYER);
      }
   }
}

void LuaNode::luaSmashed()
{
   _smashed = true;

   lua_getglobal(_lua_state, FUNCTION_SMASHED);
   const auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_SMASHED);
   }
}

/**
 * @brief LuaNode::luaSendPatrolPath sends the patrol path coordinates to the lua script
 * callback name: setPath
 */
void LuaNode::luaSendPatrolPath()
{
   if (_movement_path_px.empty())
   {
      return;
   }

   lua_getglobal(_lua_state, FUNCTION_SET_PATH);

   lua_pushstring(_lua_state, "path");
   luaSendPath(_movement_path_px);

   // vec.size + 1 args, 0 result
   const auto result = lua_pcall(_lua_state, 2, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_SET_PATH);
   }
}

/**
 * @brief LuaNode::luaDie lua script is told to die
 */
void LuaNode::die()
{
   _dead = true;

   // resetting the body will get it removed from the luainterface class
   Level::getCurrentLevel()->getWorld()->DestroyBody(_body);
   _body = nullptr;
}

/*
<map version="1.9" tiledversion="1.9.1" orientation="orthogonal" renderorder="right-down" width="2" height="2" tilewidth="24"
tileheight="24" infinite="0" nextlayerid="4" nextobjectid="11"> <tileset firstgid="1" source="enemy_spiky.tsx"/> <layer id="1" name="Tile
Layer 1" width="2" height="2"> <data encoding="csv"> 1,2, 25,26
</data>
 </layer>
 <objectgroup id="2" name="shapes">
  <object id="1" name="spikes" x="12" y="35.7205">
   <polygon points="0,0 1.11372,-5.05742
-1.91447,-12.4563 8.22697,-14.7191 12.5569,-19.9376 16.4601,-15.1081 25.9507,-13.4288 22.7777,-6.19701 24.1353,0.166728"/>
  </object>
  <object id="9" name="bottom_2" x="6" y="36" width="12" height="12">
   <ellipse/>
  </object>
  <object id="10" name="bottom_1" x="28" y="36" width="12" height="12">
   <ellipse/>
  </object>
 </objectgroup>
 <objectgroup id="3" name="hitboxes">
  <object id="6" name="hitbox_3" x="10.1504" y="16.4098" width="28.2556" height="21.6053"/>
  <object id="7" name="hitbox_2" x="4.87596" y="35.2406" width="16.1879" height="12.5827"/>
  <object id="8" name="hitbox_1" x="26.079" y="35.3816" width="16.1879" height="12.5827"/>
 </objectgroup>
</map>

 */

void LuaNode::loadShapesFromTmx(const std::string& tmxFile)
{
   TmxParser tmx_parser;
   tmx_parser.parse(tmxFile);

   const auto& elements = tmx_parser.getElements();
   for (const auto& element : elements)
   {
      if (element->_type == TmxElement::Type::TypeObjectGroup)
      {
         auto objectGroup = std::dynamic_pointer_cast<TmxObjectGroup>(element);
         for (const auto& object_pair : objectGroup->_objects)
         {
            const auto& object = object_pair.second;

            // if (object->getShape() == TmxObject::Shape::Rectangle)
            // {
            //    // Convert rectangle to Box2D rectangle
            //    addShapeRect(
            //       object->getAABB().size.x * MPP, object->getAABB().size.y * MPP, object->getAABB().position.x * MPP,
            //       object->getAABB().position.y * MPP
            //    );
            // }
            // else if (object->getShape() == TmxObject::Shape::Ellipse)
            // {
            //    // Convert ellipse to Box2D circle
            //    addShapeCircle(
            //       object->getAABB().size.x / 2 * MPP,
            //       (object->getAABB().position.x + object->getAABB().size.x / 2) * MPP,
            //       (object->getAABB().position.y + object->getAABB().size.y / 2) * MPP
            //    );
            // }
            /*else*/ if (object->_polygon != nullptr)
            {
               const auto& points = object->_polygon->_polyline;
               std::vector<b2Vec2> b2Points;
               for (const auto& point : points)
               {
                  b2Points.emplace_back(point.x * MPP, point.y * MPP);
               }
               addShapePoly(b2Points.data(), static_cast<int32_t>(b2Points.size()));
            }
            else if (object->_polyline != nullptr)
            {
               const auto& points = object->_polyline->_path;
               std::vector<b2Vec2> b2Points;
               for (const auto& point : points)
               {
                  b2Points.emplace_back(point.x * MPP, point.y * MPP);
               }
               addShapePoly(b2Points.data(), static_cast<int32_t>(b2Points.size()));
            }
         }
      }
   }
}

void LuaNode::loadHitboxesFromTmx(const std::string& tmxFile)
{
   TmxParser tmx_parser;
   tmx_parser.parse(tmxFile);

   const auto& elements = tmx_parser.getElements();
   for (const auto& element : elements)
   {
      if (element->_type == TmxElement::Type::TypeObjectGroup)
      {
         auto objectGroup = std::dynamic_pointer_cast<TmxObjectGroup>(element);
         if (objectGroup->_name == "Hitboxes")
         {
            for (const auto& object_pair : objectGroup->_objects)
            {
               const auto& object = object_pair.second;
               // if (object->getShape() == TmxObject::Shape::Rectangle)
               // {
               //    addHitbox(
               //       static_cast<int32_t>(object->getAABB().position.x),
               //       static_cast<int32_t>(object->getAABB().position.y),
               //       static_cast<int32_t>(object->getAABB().size.x),
               //       static_cast<int32_t>(object->getAABB().size.y)
               //    );
               // }
            }
         }
      }
   }
}

/**
 * @brief LuaNode::luaMovedTo tell lua script where the engine moved it to
 * callback name: movedTo
 * lua param x: x position (double)
 * lua param y: y position (double)
 */
void LuaNode::luaMovedTo()
{
   const auto x = _position_px.x;
   const auto y = _position_px.y;

   lua_getglobal(_lua_state, FUNCTION_MOVED_TO);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, static_cast<double>(x));
      lua_pushnumber(_lua_state, static_cast<double>(y));

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_MOVED_TO);
      }
   }
}

/**
 * @brief LuaNode::luaSetStartPosition
 * callback name: setStartPosition
 * lua param x: x position of start position (double)
 * lua param y: y position of start position (double)
 */
void LuaNode::luaSetStartPosition()
{
   const auto x = _start_position_px.x;
   const auto y = _start_position_px.y;

   lua_getglobal(_lua_state, FUNCTION_SET_START_POSITION);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, static_cast<double>(x));
      lua_pushnumber(_lua_state, static_cast<double>(y));

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_SET_START_POSITION);
      }
   }
}

/**
 * @brief LuaNode::luaPlayerMovedTo engine moved player to a certain position
 * callback name: playerMovedTo
 * lua param x: x position of player position (double)
 * lua param y: y position of player position (double)
 */
void LuaNode::luaPlayerMovedTo()
{
   const auto& pos = Player::getCurrent()->getPixelPositionFloat();

   lua_getglobal(_lua_state, FUNCTION_PLAYER_MOVED_TO);

   if (lua_isfunction(_lua_state, -1))
   {
      lua_pushnumber(_lua_state, pos.x);
      lua_pushnumber(_lua_state, pos.y);

      const auto result = lua_pcall(_lua_state, 2, 0, 0);

      if (result != LUA_OK)
      {
         LuaNodeCallbacks::error(_lua_state, FUNCTION_PLAYER_MOVED_TO);
      }
   }
}

/**
 * @brief LuaNode::luaRetrieveProperties instruct lua node to retrieve properties now
 * callback name: retrieveProperties
 */
void LuaNode::luaRetrieveProperties()
{
   lua_getglobal(_lua_state, FUNCTION_RETRIEVE_PROPERTIES);

   // 0 args, 0 result
   const auto result = lua_pcall(_lua_state, 0, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_RETRIEVE_PROPERTIES);
   }
}

/**
 * @brief LuaNode::luaTimeout timeout timer fired
 * @param timerId timer id of timeout timer
 * callback name: timeout
 * lua param timerId: id of timeout timer
 */
void LuaNode::luaTimeout(int32_t timerId)
{
   lua_getglobal(_lua_state, FUNCTION_TIMEOUT);
   lua_pushinteger(_lua_state, timerId);

   const auto result = lua_pcall(_lua_state, 1, 0, 0);

   if (result != LUA_OK)
   {
      LuaNodeCallbacks::error(_lua_state, FUNCTION_TIMEOUT);
   }
}

/**
 * @brief LuaNode::luaSendPath inject a path into the current lua state
 * @param vec vector of 2d vectors
 */
void LuaNode::luaSendPath(const std::vector<sf::Vector2f>& vec)
{
   lua_newtable(_lua_state);

   int32_t i = 0;
   for (const auto& v : vec)
   {
      lua_pushnumber(_lua_state, v.x);  // push x
      lua_rawseti(_lua_state, -2, ++i);
      lua_pushnumber(_lua_state, v.y);  // push y
      lua_rawseti(_lua_state, -2, ++i);
   }
}

void LuaNode::damagePlayerInRadius(int32_t damage, float x, float y, float radius)
{
   sf::Vector2f node_position{x, y};
   const auto player_position = Player::getCurrent()->getPixelPositionFloat();

   auto dist = (player_position - node_position);
   auto len = SfmlMath::length(dist);

   if (len <= radius)
   {
      // does it really make sense to normalize this vector?
      Player::getCurrent()->damage(damage, SfmlMath::normalize(-dist));
   }
}

void LuaNode::damagePlayer(int32_t damage, float forceX, float forceY)
{
   Player::getCurrent()->damage(damage, sf::Vector2f(forceX, forceY));
}

b2Vec2 LuaNode::getLinearVelocity() const
{
   b2Vec2 velocity;
   velocity.SetZero();

   if (_body)
   {
      velocity = _body->GetLinearVelocity();
   }

   return velocity;
}

void LuaNode::setLinearVelocity(const b2Vec2& vel)
{
   if (!_body)
   {
      return;
   }

   _body->SetLinearVelocity(vel);
}

void LuaNode::applyLinearImpulse(const b2Vec2& vel)
{
   if (!_body)
   {
      return;
   }

   _body->ApplyLinearImpulse(vel, _body->GetWorldCenter(), true);
}

void LuaNode::applyForce(const b2Vec2& force)
{
   if (!_body)
   {
      return;
   }

   _body->ApplyForceToCenter(force, true);
}

void LuaNode::boom(float x, float y, float intensity)
{
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, BoomSettings{intensity, 1.0f});
}

void LuaNode::playDetonationAnimationHuge(float x, float y)
{
   auto detonation = DetonationAnimation::makeHugeExplosion(sf::Vector2f{x, y});
   AnimationPlayer::getInstance().add(detonation.getAnimations());
}

void LuaNode::playDetonationAnimation(const std::vector<DetonationAnimation::DetonationRing>& rings)
{
   DetonationAnimation detonation{rings};
   AnimationPlayer::getInstance().add(detonation.getAnimations());
}

void LuaNode::setGravityScale(float scale)
{
   _body->SetGravityScale(scale);
}

void LuaNode::setTransform(const b2Vec2& position, float angle)
{
   _body->SetTransform(position, angle);
}

void LuaNode::addSprite()
{
   auto sprite = std::make_unique<sf::Sprite>(*_texture);
   _sprites.emplace_back(std::move(sprite));
   _sprite_offsets_px.emplace_back();
}

void LuaNode::setSpriteOrigin(int32_t id, float x, float y)
{
   _sprites[id]->setOrigin({x, y});
}

void LuaNode::setSpriteOffset(int32_t id, float x, float y)
{
   _sprite_offsets_px[id].x = x;
   _sprite_offsets_px[id].y = y;
}

void LuaNode::setActive(bool active)
{
   _body->SetEnabled(active);
}

void LuaNode::setDamageToPlayer(int32_t damage)
{
   for (auto fixture = _body->GetFixtureList(); fixture; fixture = fixture->GetNext())
   {
      auto user_data = fixture->GetUserData().pointer;
      if (!user_data)
      {
         continue;
      }

      auto fixture_node = static_cast<FixtureNode*>(user_data);
      fixture_node->setProperty("damage", damage);
   }
}

void LuaNode::makeDynamic()
{
   _body->SetType(b2_dynamicBody);
}

void LuaNode::makeStatic()
{
   _body->SetType(b2_staticBody);
}

class LuaQueryCallback : public b2QueryCallback
{
public:
   std::vector<b2Body*> _bodies;

   bool ReportFixture(b2Fixture* fixture) override
   {
      _bodies.push_back(fixture->GetBody());

      // to keep going to find all fixtures in the query area
      return true;
   }
};

int32_t LuaNode::queryAABB(const b2AABB& aabb)
{
   LuaQueryCallback query_callback;
   Level::getCurrentLevel()->getWorld()->QueryAABB(&query_callback, aabb);

   // Log::Info() << queryCallback.mBodies.size();
   return static_cast<int32_t>(query_callback._bodies.size());
}

class LuaRaycastCallback : public b2RayCastCallback
{
public:
   std::vector<b2Body*> _bodies;

   float ReportFixture(
      b2Fixture* fixture,
      const b2Vec2& /*point*/,
      const b2Vec2& /*normal*/,
      float /*fraction*/
   ) override
   {
      _bodies.push_back(fixture->GetBody());
      return 0.0f;
   }
};

int32_t LuaNode::queryRaycast(const b2Vec2& point1, const b2Vec2& point2)
{
   LuaRaycastCallback query_callback;
   Level::getCurrentLevel()->getWorld()->RayCast(&query_callback, point1, point2);

   // Log::Info() << queryCallback.mBodies.size();
   return static_cast<int32_t>(query_callback._bodies.size());
}

bool LuaNode::getPropertyBool(const std::string& key, bool default_value)
{
   auto value = default_value;
   auto it = _properties.find(key);
   if (it != _properties.end())
   {
      value = std::get<bool>(it->second);
   }
   return value;
}

double LuaNode::getPropertyDouble(const std::string& key, double default_value)
{
   auto value = default_value;
   auto it = _properties.find(key);
   if (it != _properties.end())
   {
      value = std::get<double>(it->second);
   }
   return value;
}

int64_t LuaNode::getPropertyInt64(const std::string& key, int64_t default_value)
{
   auto value = default_value;
   auto it = _properties.find(key);
   if (it != _properties.end())
   {
      value = std::get<int64_t>(it->second);
   }
   return value;
}

void LuaNode::setupBody()
{
   const auto static_body = getPropertyBool("static_body");
   const auto restitution = static_cast<float>(getPropertyDouble("restitution", 0.0f));
   const auto density = static_cast<float>(getPropertyDouble("density", 1.0f));
   const auto friction = static_cast<float>(getPropertyDouble("friction", 0.0f));
   const auto damage = static_cast<int32_t>(getPropertyInt64("damage"));
   const auto sensor = static_cast<bool>(getPropertyBool("sensor"));
   const auto collides_with_player = static_cast<bool>(getPropertyBool("collides_with_player", true));
   const auto mask_bits = getPropertyBool("walk_through", true);
   const auto restitution_threshold = static_cast<float>(getPropertyDouble("restitution_threshold", 1.0f * b2_lengthUnitsPerMeter));

   _body->SetTransform(b2Vec2{_start_position_px.x * MPP, _start_position_px.y * MPP}, 0.0f);
   _body->SetFixedRotation(true);
   _body->SetType(static_body ? b2_staticBody : b2_dynamicBody);

   for (auto shape : _shapes_m)
   {
      b2FixtureDef fd;
      fd.density = density;
      fd.friction = friction;
      fd.restitution = restitution;
      fd.restitutionThreshold = restitution_threshold;
      fd.shape = shape;

      // apply default filter
      // http://www.iforce2d.net/b2dtut/collision-filtering
      fd.filter.categoryBits = (mask_bits ? category_bits_default : CategoryEnemyCollideWith);
      fd.filter.maskBits = (collides_with_player ? mask_bits_default : CategoryBoundary);
      fd.filter.groupIndex = group_index_default;

      auto fixture = _body->CreateFixture(&fd);
      auto fixture_node = new FixtureNode(this);
      fixture_node->setType(ObjectTypeEnemy);
      fixture_node->setProperty("damage", damage);
      fixture_node->setCollisionCallback([this]() { luaCollisionWithPlayer(); });
      fixture->SetUserData(static_cast<void*>(fixture_node));

      if (sensor)
      {
         fixture->SetSensor(true);
      }
   }

   // mBody->Dump();
}

void LuaNode::addShapeCircle(float radius, float x, float y)
{
   auto shape = new b2CircleShape();
   shape->m_p.Set(x, y);
   shape->m_radius = radius;
   _shapes_m.push_back(shape);
}

void LuaNode::addShapeRect(float width, float height, float x, float y)
{
   auto shape = new b2PolygonShape();
   shape->SetAsBox(width, height, b2Vec2(x, y), 0.0f);
   _shapes_m.push_back(shape);
}

void LuaNode::addShapeRectBevel(float width, float height, float bevel, float offset_x, float offset_y)
{
   auto shape = new b2PolygonShape();

   std::array<b2Vec2, 8> vertices{
      b2Vec2{bevel + offset_x, 0.0f + offset_y},
      b2Vec2{0.0f + offset_x, bevel + offset_y},
      b2Vec2{0.0f + offset_x, height - bevel + offset_y},
      b2Vec2{bevel + offset_x, height + offset_y},
      b2Vec2{width - bevel + offset_x, height + offset_y},
      b2Vec2{width + offset_x, height - bevel + offset_y},
      b2Vec2{width + offset_x, bevel + offset_y},
      b2Vec2{width - bevel + offset_x, 0.0f + offset_y},
   };

   shape->Set(vertices.data(), static_cast<int32_t>(vertices.size()));
   _shapes_m.push_back(shape);
}

void LuaNode::addShapePoly(const b2Vec2* points, int32_t size)
{
   auto shape = new b2PolygonShape();
   shape->Set(points, size);
   _shapes_m.push_back(shape);
}

void LuaNode::addWeapon(const std::shared_ptr<Weapon>& weapon)
{
   _weapons.push_back(weapon);
}

void LuaNode::useWeapon(size_t index, b2Vec2 from, b2Vec2 to)
{
   auto& gun = dynamic_cast<Gun&>(*_weapons[index]);
   gun.setParentAudioUpdateData(_audio_update_data);
   gun.useInIntervals(Level::getCurrentLevel()->getWorld(), from, to);
}

void LuaNode::addPlayerSkill(int32_t skill_type)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(skill_type);
}

void LuaNode::removePlayerSkill(int32_t skill_type)
{
   SaveState::getPlayerInfo()._extra_table._skills._skills &= ~static_cast<int32_t>(skill_type);
}

void LuaNode::stopScript()
{
   if (_lua_state)
   {
      lua_close(_lua_state);
      _lua_state = nullptr;
   }
}

int32_t LuaNode::getDamageFromPlayer() const
{
   return _damage_from_player;
}

const std::optional<LuaNode::HighResTimePoint> LuaNode::getHitTime() const
{
   return _hit_time;
}

void LuaNode::updateVelocity()
{
   if (!_body)
   {
      return;
   }

   std::optional<double> velocity_max;
   std::optional<double> acceleration;

   const auto velocity_it = _properties.find("velocity_walk_max");
   if (velocity_it != _properties.end())
   {
      velocity_max = *std::get_if<double>(&(velocity_it->second));
   }

   const auto acceleration_it = _properties.find("acceleration_ground");
   if (acceleration_it != _properties.end())
   {
      acceleration = *std::get_if<double>(&(acceleration_it->second));
   }

   if (!velocity_max.has_value())
   {
      return;
   }

   if (!acceleration.has_value())
   {
      return;
   }

   auto desired_velocity = 0.0f;
   const auto velocity = _body->GetLinearVelocity();

   if (_keys_pressed & KeyPressedLeft)
   {
      desired_velocity = static_cast<float>(b2Max(velocity.x - acceleration.value(), -velocity_max.value()));
   }

   if (_keys_pressed & KeyPressedRight)
   {
      desired_velocity = static_cast<float>(b2Min(velocity.x + acceleration.value(), velocity_max.value()));
   }

   // calc impulse, disregard time factor
   const auto velocity_change = desired_velocity - velocity.x;
   const auto impulse = _body->GetMass() * velocity_change;

   _body->ApplyLinearImpulse(b2Vec2(impulse, 0.0f), _body->GetWorldCenter(), true);
}

void LuaNode::updateWeapons(const sf::Time& dt)
{
   for (auto& w : _weapons)
   {
      w->update({dt, Level::getCurrentLevel()->getWorld()});
   }
}

void LuaNode::updateHitboxOffsets()
{
   for (auto& hitbox : _hitboxes)
   {
      hitbox._rect_px.position.x = _position_px.x;
      hitbox._rect_px.position.y = _position_px.y;
   }
}

void LuaNode::updatePosition()
{
   if (!_body)
   {
      return;
   }

   auto x_px = _body->GetPosition().x * PPM;
   auto y_px = _body->GetPosition().y * PPM;

   _position_px.x = x_px;
   _position_px.y = y_px;

   updateHitboxOffsets();
}

void LuaNode::updateSpriteRect(int32_t id, int32_t x_px, int32_t y_px, int32_t w_px, int32_t h_px)
{
   _sprites[id]->setTextureRect(sf::IntRect({x_px, y_px}, {w_px, h_px}));
}


void LuaNode::setSpriteScale(int32_t id, float x_scale, float y_scale)
{
   _sprites[id]->setScale({x_scale, y_scale});
}

void LuaNode::setSpriteColor(int32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
   _sprites[id]->setColor({r, g, b, a});
}

void LuaNode::setSpriteVisible(int32_t id, bool visible)
{
   if (id >= 0 && id < static_cast<int32_t>(_sprites.size()))
   {
      sf::Color current_color = _sprites[id]->getColor();
      current_color.a = visible ? 255 : 0;
      _sprites[id]->setColor(current_color);
   }
}

void LuaNode::setVisible(bool visible)
{
   _visible = visible;
}

void LuaNode::setKeysPressed(int32_t keys)
{
   _keys_pressed = keys;
}

void LuaNode::updateDebugRect(int32_t index, float left_px, float top_px, float width_px, float height_px)
{
   auto& rect = _debug_rects[index];
   rect.position.x = left_px;
   rect.position.y = top_px;
   rect.size.x = width_px;
   rect.size.y = height_px;
}

void LuaNode::addDebugRect()
{
   _debug_rects.emplace_back();
}

void LuaNode::addHitbox(int32_t left_px, int32_t top_px, int32_t width_px, int32_t height_px)
{
   sf::FloatRect rect{{_position_px.x, _position_px.y}, {static_cast<float>(width_px), static_cast<float>(height_px)}};
   sf::Vector2f offset{static_cast<float>(left_px), static_cast<float>(top_px)};
   Hitbox box{rect, offset};
   _hitboxes.push_back(box);

   // re-calculate bounding box
   auto left = box.getRectTranslated().position.x;
   auto right = box.getRectTranslated().position.x + box.getRectTranslated().size.x;
   auto top = box.getRectTranslated().position.y;
   auto bottom = box.getRectTranslated().position.y + box.getRectTranslated().size.y;
   for (const auto& hitbox : _hitboxes)
   {
      const auto other = hitbox.getRectTranslated();
      left = std::min(left, other.position.x);
      top = std::min(top, other.position.y);
      right = std::max(right, other.position.x + other.size.x);
      bottom = std::max(bottom, other.position.y + other.size.y);
   }

   sf::FloatRect bounding_box;
   bounding_box.position.x = left;
   bounding_box.position.y = top;
   bounding_box.size.x = right - left;
   bounding_box.size.y = bottom - top;

   _bounding_box = bounding_box;

   // even though the node might probably be moving, it's safe to used a fixed chunk with the current bounding box
   _chunks.clear();
   addChunks(bounding_box);
}

void LuaNode::addAudioRange(float far_distance, float far_volume, float near_distance, float near_volume)
{
   AudioRange audio_range;
   audio_range._radius_far_px = far_distance;
   audio_range._volume_far = far_volume;
   audio_range._radius_near_px = near_distance;
   audio_range._volume_near = near_volume;
   _audio_update_data._range = audio_range;

   if (_hitboxes.empty())
   {
      Log::Warning() << "no hitboxes defined for " << _script_name;
   }
}

void LuaNode::addSample(const std::string& sample)
{
   Audio::getInstance().addSample(sample);
   _has_audio = true;
}

void LuaNode::playSample(const std::string& sample, float volume)
{
   if (!_audio_enabled)
   {
      return;
   }

   Audio::getInstance().playSample({sample, volume * _reference_volume});
}

bool LuaNode::intersectsPlayer(float x, float y, float width, float height)
{
   sf::FloatRect rect{{x, y}, {width, height}};
   const auto player_rect = Player::getCurrent()->getPixelRectFloat();
   return player_rect.findIntersection(rect).has_value();
}

bool LuaNode::checkPlayerDead() const
{
   const auto player = Player::getCurrent();
   return player ? player->isDead() : false;
}

bool LuaNode::isPhysicsPathClear(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const
{
   return !Level::getCurrentLevel()->isPhysicsPathClear({x0, y0}, {x1, y1});
}

float LuaNode::getWorldGravity() const
{
   return PhysicsConfiguration::getInstance()._gravity;
}

void LuaNode::addWeaponFromScript(
   WeaponType weapon_type,
   int fire_interval,
   int damage_value,
   float gravity_scale,
   float radius,
   const std::vector<b2Vec2>& polygon_points
)
{
   std::unique_ptr<b2Shape> shape;

   // add weapon with projectile radius only
   if (!polygon_points.empty())
   {
      shape = std::make_unique<b2PolygonShape>();
      auto poly = new b2Vec2[polygon_points.size()];
      for (size_t i = 0; i < polygon_points.size(); i++)
      {
         poly[i] = polygon_points[i];
      }
      dynamic_cast<b2PolygonShape*>(shape.get())->Set(poly, static_cast<int32_t>(polygon_points.size()));
   }
   else
   {
      shape = std::make_unique<b2CircleShape>();
      dynamic_cast<b2CircleShape*>(shape.get())->m_radius = radius;
   }

   WeaponProperties properties;
   properties._parent_body = _body;
   properties._shape = std::move(shape);

   properties._properties["damage"] = damage_value;
   properties._properties["use_interval_ms"] = fire_interval;
   properties._properties["gravity_scale"] = gravity_scale;

   if (weapon_type != WeaponType::None)
   {
      auto weapon = WeaponFactory::create(weapon_type, properties);
      addWeapon(std::move(weapon));
   }
   else
   {
      Log::Fatal() << _script_name << " tried to set an invalid weapon";
   }
}

void LuaNode::setProjectileTexture(uint32_t weapon_index, const std::string& path, const sf::Rect<int32_t>& rect)
{
   const auto& texture = TexturePool::getInstance().get(path);
   dynamic_cast<Gun&>(*_weapons[weapon_index]).setProjectileAnimation(texture, rect);
}

void LuaNode::setProjectileAnimation(
   uint32_t weapon_index,
   const std::string& path,
   uint32_t frame_width,
   uint32_t frame_height,
   float frame_origin_x,
   float frame_origin_y,
   float time_per_frame_s,
   uint32_t frame_count,
   uint32_t frames_per_row,
   uint32_t start_frame
)
{
   const auto texture = TexturePool::getInstance().get(path);
   const sf::Vector2f frame_origin{frame_origin_x, frame_origin_y};

   // assume identical frame times for now
   std::vector<sf::Time> frame_times_s;
   for (auto i = 0u; i < frame_count; i++)
   {
      frame_times_s.push_back(sf::seconds(time_per_frame_s));
   }

   AnimationFrameData frame_data(texture, frame_origin, frame_width, frame_height, frame_count, frames_per_row, frame_times_s, start_frame);

   dynamic_cast<Gun&>(*_weapons[weapon_index]).setProjectileAnimation(frame_data);
}

void LuaNode::startTimer(int32_t delay, int32_t timer_id)
{
   Timer::add(
      std::chrono::milliseconds(delay),
      [this, timer_id]() { luaTimeout(timer_id); },
      Timer::Type::Singleshot,
      Timer::Scope::UpdateIngame
   );
}

void LuaNode::registerHitAnimation(
   uint32_t weapon_index,
   const std::string& path,
   uint32_t frame_width,
   uint32_t frame_height,
   float time_per_frame_s,
   uint32_t frame_count,
   uint32_t frames_per_row,
   uint32_t start_frame
)
{
   ProjectileHitAnimation::addReferenceAnimation(
      path,
      frame_width,
      frame_height,
      std::chrono::duration<float, std::chrono::seconds::period>{time_per_frame_s},
      frame_count,
      frames_per_row,
      start_frame
   );

   dynamic_cast<Gun&>(*_weapons[weapon_index]).setProjectileIdentifier(path);
}

void LuaNode::registerHitSamples(const std::string& path, const std::vector<std::pair<std::string, float>>& samples)
{
   std::vector<ProjectileHitAudio::ProjectileHitSample> hit_samples;
   for (const auto& sample : samples)
   {
      hit_samples.push_back({sample.first, sample.second});
   }
   ProjectileHitAudio::addReferenceSamples(path, hit_samples);
}

void LuaNode::playDetonationAnimationFromScript(
   float x,
   float y,
   const std::vector<DetonationAnimation::DetonationRing>& rings
)
{
   if (!rings.empty())
   {
      playDetonationAnimation(rings);
   }
   else if (x != 0.0f || y != 0.0f)
   {
      playDetonationAnimationHuge(x, y);
   }
}

void LuaNode::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   if (!_visible)
   {
      return;
   }

   if (_hit_time.has_value())
   {
      const auto hit_duration_s = (std::chrono::high_resolution_clock::now() - _hit_time.value());
      constexpr auto hit_duration_max_s = 0.3f;
      if (hit_duration_s.count() > hit_duration_max_s)
      {
         _hit_time.reset();
         _hit_flash = 0.0f;
      }
      else
      {
         _hit_flash = 1.0f - (hit_duration_s.count() / hit_duration_max_s);
      }

      _flash_shader.setUniform("flash", _hit_flash);
   }

   // draw sprite on top of projectiles
   for (auto& weapon : _weapons)
   {
      weapon->draw(target);
   }

   for (auto i = 0u; i < _sprites.size(); i++)
   {
      auto& sprite = _sprites[i];

      if (sprite->getColor().a == 0)
      {
         continue;
      }

      const auto& offset = _sprite_offsets_px[i];
      const auto center = sf::Vector2f(sprite->getTextureRect().size.x / 2.0f, sprite->getTextureRect().size.y / 2.0f);
      sprite->setPosition(_position_px - center + offset);
      target.draw(*sprite, &_flash_shader);
   }

   // draw debug rectangles if they were added
   for (const auto& debug_rect : _debug_rects)
   {
      DebugDraw::drawRect(target, debug_rect);
   }
}

std::optional<sf::FloatRect> LuaNode::getBoundingBoxPx()
{
   return _bounding_box;
}

bool LuaNode::isDestructible() const
{
   return true;
}

const std::vector<Hitbox>& LuaNode::getHitboxes()
{
   return _hitboxes;
}
