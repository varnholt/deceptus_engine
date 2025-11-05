// base
#include "game/mechanisms/movingplatform.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h" 0
#include "game/player/player.h"

#include <cmath>
#include <numbers>

#include <box2d/box2d.h>

namespace
{

double cosineInterpolate(double y1, double y2, double mu)
{
   const auto mu2 = (1.0 - cos(mu * std::numbers::pi)) * 0.5;
   return ((y1 * (1.0 - mu2)) + (y2 * mu2));
}

const auto registered_moving_platform = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("MovingPlatform", "moving_platforms");

   registry.registerLayerName(
      "moving_platforms",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MovingPlatform>(parent);
         mechanism->setup(data);
         mechanisms["moving_platforms"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "MovingPlatform",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MovingPlatform>(parent);
         mechanism->setup(data);
         mechanisms["moving_platforms"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
constexpr auto element_width_m = PIXELS_PER_TILE / PPM;
constexpr auto element_height_m = 0.5f * PIXELS_PER_TILE / PPM;
}  // namespace

MovingPlatform::MovingPlatform(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(MovingPlatform).name());
}

std::string_view MovingPlatform::objectName() const
{
   return "MovingPlatform";
}

void MovingPlatform::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   for (auto& sprite : _sprites)
   {
      sprite.setTexture(*_texture_map);
   }

   for (const auto& sprite : _sprites)
   {
      color.draw(sprite);
   }

   if (_normal_map)
   {
      for (auto& sprite : _sprites)
      {
         sprite.setTexture(*_normal_map);
      }

      for (const auto& sprite : _sprites)
      {
         normal.draw(sprite);
      }
   }
}

const std::vector<sf::Vector2f>& MovingPlatform::getPixelPath() const
{
   return _pixel_path;
}

float MovingPlatform::getDx() const
{
   return _pos.x - _pos_prev.x;
}

b2Body* MovingPlatform::getBody()
{
   return _body;
}

void MovingPlatform::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);

   if (_initialized)
   {
      _lever_lag = enabled ? 0.0f : 1.0f;
   }
   else
   {
      _initialized = true;
   }
}

void MovingPlatform::setupTransform()
{
   auto pos_x_m = static_cast<float>(_tile_positions.x) * PIXELS_PER_TILE / PPM;
   auto pos_y_m = static_cast<float>(_tile_positions.y) * PIXELS_PER_TILE / PPM;
   _body->SetTransform(b2Vec2(pos_x_m, pos_y_m), 0);
}

void MovingPlatform::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;

   std::array<b2Vec2, 4> vertices{
      b2Vec2(0, 0),
      b2Vec2(0, element_height_m),
      b2Vec2(element_width_m * static_cast<float>(_platform_width_tl), element_height_m),
      b2Vec2(element_width_m * static_cast<float>(_platform_width_tl), 0)
   };

   polygon_shape.Set(vertices.data(), 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   auto* fixture = _body->CreateFixture(&polygon_shape, 0);
   auto* object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMovingPlatform);
   fixture->SetUserData(static_cast<void*>(object_data));
}

void MovingPlatform::addSprite(const sf::Sprite& sprite)
{
   _sprites.push_back(sprite);
}

void MovingPlatform::setup(const GameDeserializeData& data)
{
   // need properties to load the platform
   if (!data._tmx_object || !data._tmx_object->_properties)
   {
      return;
   }

   if (!data._tmx_object->_polyline)
   {
      return;
   }

   const auto& path = data._tmx_object->_polyline->_path;

   // load textures
   const auto texture_path = data._base_path / "tilesets" / "platforms.png";
   const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
   const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);
   if (std::filesystem::exists(normal_map_path))
   {
      _normal_map = TexturePool::getInstance().get(normal_map_path);
   }

   _texture_map = TexturePool::getInstance().get(texture_path);

   // read properties
   const auto& map = data._tmx_object->_properties->_map;
   _z_index = ValueReader::readValue<int32_t>("z", map).value_or(10);
   _platform_width_tl = ValueReader::readValue<int32_t>("platform_width_tl", map).value_or(4);

   // animation
   //
   // uneven tile count
   //
   //    +-----+-----+-----+-----+-----+
   //    |     |     |#####|     |     |
   //    |     |     |#####|     |     |
   //    +-----+-----+-----+-----+-----+
   //                   ^ animate this guy
   //
   //
   // even tile count
   //
   //    +-----+-----+-----+-----+-----+-----+
   //    |     |     |#####|#####|     |     |
   //    |     |     |#####|#####|     |     |
   //    +-----+-----+-----+-----+-----+-----+
   //                   ^     ^ animate these guys

   if (_platform_width_tl % 2 != 0)
   {
      _animated_tile_index_0 = ((_platform_width_tl + 1) / 2) - 1;
   }
   else
   {
      _animated_tile_index_0 = (_platform_width_tl / 2) - 1;
      _animated_tile_index_1 = (_platform_width_tl / 2);
   }

   for (auto i = 0; i < _platform_width_tl; i++)
   {
      auto tu_tl = 0;
      constexpr auto tv_tl = 0;

      if (_platform_width_tl > 2)
      {
         if (i == 0)  // first tile
         {
            tu_tl = 4;
         }
         else if (i == _platform_width_tl - 1)  // last tile
         {
            tu_tl = 7;
         }
         else  // other tiles
         {
            tu_tl = 5 + std::rand() % 1;  // or 6
         }
      }
      else if (_platform_width_tl == 2)
      {
         if (i == 0)
         {
            tu_tl = 0;
         }
         else if (i == 1)
         {
            tu_tl = 1;
         }
      }

      sf::Sprite sprite(*_texture_map);
      sprite.setTextureRect(
         sf::IntRect(
            {tu_tl * PIXELS_PER_TILE, tv_tl * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE * 2}
            // 1 platform tile and one background tile for perspective
         )
      );

      addSprite(sprite);
   }

   b2Vec2 platform_pos_m{};
   auto path_index = 0.0f;

   auto platform_x_min_m = std::numeric_limits<float>::max();
   auto platform_y_min_m = std::numeric_limits<float>::max();
   auto platform_x_max_m = std::numeric_limits<float>::min();
   auto platform_y_max_m = std::numeric_limits<float>::min();

   const auto path_size = static_cast<float>(path.size() - 1);
   for (const auto& poly_pos_px : path)
   {
      const auto time = path_index / path_size;

      // we don't want to position the platform right on the path, we only
      // want to move its center there
      const auto half_width_px = (static_cast<float>(_platform_width_tl) * PIXELS_PER_TILE / 2.0f);
      const auto x_px = (data._tmx_object->_x_px + poly_pos_px.x) - half_width_px;
      const auto y_px = (data._tmx_object->_y_px + poly_pos_px.y);

      platform_pos_m.x = x_px * MPP;
      platform_pos_m.y = y_px * MPP;

      _interpolation.addKey(platform_pos_m, time);
      _pixel_path.emplace_back((data._tmx_object->_x_px + poly_pos_px.x), (data._tmx_object->_y_px + poly_pos_px.y));

      platform_x_min_m = std::min(platform_pos_m.x, platform_x_min_m);
      platform_y_min_m = std::min(platform_pos_m.y, platform_y_min_m);
      platform_x_max_m = std::max(platform_pos_m.x, platform_x_max_m);
      platform_y_max_m = std::max(platform_pos_m.y, platform_y_max_m);

      path_index += 1.0f;
   }

   setupBody(data._world);
   _body->SetTransform(platform_pos_m, 0.0f);

   // set up bounding rect
   _rect.position.x = platform_x_min_m * PPM;
   _rect.position.y = platform_y_min_m * PPM;
   _rect.size.x = static_cast<float>(_platform_width_tl) * PIXELS_PER_TILE;
   _rect.size.y = (platform_y_max_m - platform_y_min_m) * PPM;

   // TODO: disabled for now
   // addChunks(_rect);
}

//  |                 |
//  |              ____
//  |        __----
//  _____----         |
//                    |
//  +-----------------+
//  0                 1
//
//  p0                pn

void MovingPlatform::updateLeverLag(const sf::Time& delta_time)
{
   if (!isEnabled())
   {
      if (_lever_lag <= 0.0f)
      {
         _lever_lag = 0.0f;
      }
      else
      {
         _lever_lag -= delta_time.asSeconds();
      }
   }
   else
   {
      if (_lever_lag < 1.0f)
      {
         _lever_lag += delta_time.asSeconds();
      }
      else
      {
         _lever_lag = 1.0f;
      }
   }
}

void MovingPlatform::update(const sf::Time& delta_time)
{
   updateLeverLag(delta_time);
   _interpolation.update(_body->GetPosition());
   const auto previous_velocity = _velocity;
   _velocity = _lever_lag * TIMESTEP_ERROR * (PPM / 60.0f) * _interpolation.getVelocity();

   // if player is standing on platform and the platform changes its direction in an instant,
   // set the player velocity to the linear platform velocity, so he doesn't jump up for a second
   if (std::signbit(previous_velocity.y) != std::signbit(_velocity.y))
   {
      if (Player::getCurrent()->getPlatform().isOnPlatform())
      {
         Player::getCurrent()->getBody()->SetLinearVelocity(_velocity);
      }
   }

   _body->SetLinearVelocity(_velocity);
   _pos_prev = _pos;
   _pos.x = _body->GetPosition().x;
   _pos.y = _body->GetPosition().y;

   auto& platform = Player::getCurrent()->getPlatform();
   if (platform.getPlatformBody() == _body)
   {
      platform.setPlatformDx(getDx());
      platform.setGravityScale(10.0f);
   }

   // update sprite animation
   //
   //   0123 4567
   // 0 aaaa lmmr    wheel animation for uneven tile count, uneven tiles
   // 1
   // 2 aaaa aaaa    wheel animation for even tile count, pairs of two
   // 3
   // 4 aaaa aaaa    wheel animation for 2 pair tile count
   // 5
   auto sprite_index = 0;
   auto horizontal = (_platform_width_tl > 1) ? 1 : 0;

   constexpr auto animation_tile_count = 4;
   constexpr auto animation_speed_factor = 10.0f;
   _animation_elapsed += _lever_lag * delta_time.asSeconds() * animation_speed_factor;
   const auto animation_tile_index = static_cast<int32_t>(_animation_elapsed) % animation_tile_count;

   for (auto& sprite : _sprites)
   {
      const auto pos_body_x_px = (_body->GetPosition().x * PPM) + (horizontal * sprite_index * PIXELS_PER_TILE);
      const auto pos_body_y_px = (_body->GetPosition().y * PPM) - PIXELS_PER_TILE;  // there's one tile offset for the perspective tile

      sprite.setPosition({pos_body_x_px, pos_body_y_px});
      auto update_sprite_rect = false;
      auto texture_u = 0;
      auto texture_v = 0;

      if (_sprites.size() == 2)
      {
         update_sprite_rect = true;
         texture_u = (animation_tile_index * 2 + sprite_index) * PIXELS_PER_TILE;
         texture_v = PIXELS_PER_TILE * 4;
      }
      else if (_sprites.size() > 2)
      {
         if (_sprites.size() % 2 == 0)  // handle even tile counts
         {
            if (sprite_index == ((_sprites.size() + 1) / 2) - 1)
            {
               update_sprite_rect = true;
               texture_u = (animation_tile_index * 2) * PIXELS_PER_TILE;
               texture_v = PIXELS_PER_TILE * 2;
            }
            else if (sprite_index == ((_sprites.size() + 1) / 2))
            {
               update_sprite_rect = true;
               texture_u = (animation_tile_index * 2 + 1) * PIXELS_PER_TILE;
               texture_v = PIXELS_PER_TILE * 2;
            }
         }
         else  // handle uneven tile counts
         {
            if (sprite_index == ((_sprites.size() + 1) / 2) - 1)
            {
               update_sprite_rect = true;
               texture_u = animation_tile_index * PIXELS_PER_TILE;
               texture_v = 0;
            }
         }
      }

      if (update_sprite_rect)
      {
         sprite.setTextureRect({{texture_u, texture_v}, {PIXELS_PER_TILE, PIXELS_PER_TILE * 2}});
      }

      sprite_index++;
   }
}

std::optional<sf::FloatRect> MovingPlatform::getBoundingBoxPx()
{
   return _rect;
}
