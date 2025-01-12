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
#include "game/level/fixturenode.h"
#include "game/player/player.h"

#include <cmath>
#include <numbers>

#include <box2d/box2d.h>

namespace
{
constexpr auto element_width_m = PIXELS_PER_TILE / PPM;
constexpr auto element_height_m = 0.5f * PIXELS_PER_TILE / PPM;
}  // namespace

MovingPlatform::MovingPlatform(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(MovingPlatform).name());
}

void MovingPlatform::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   for (auto& sprite : _sprites)
   {
      sprite.setTexture(*_texture_map.get());
   }

   for (const auto& sprite : _sprites)
   {
      color.draw(sprite);
   }

   for (auto& sprite : _sprites)
   {
      sprite.setTexture(*_normal_map.get());
   }

   for (const auto& sprite : _sprites)
   {
      normal.draw(sprite);
   }
}

double MovingPlatform::cosineInterpolate(double y1, double y2, double mu)
{
   const auto mu2 = (1.0 - cos(mu * std::numbers::pi)) * 0.5;
   return (y1 * (1.0 - mu2) + y2 * mu2);
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
   auto x = _tile_positions.x * PIXELS_PER_TILE / PPM;
   auto y = _tile_positions.y * PIXELS_PER_TILE / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

void MovingPlatform::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0, 0);
   vertices[1] = b2Vec2(0, element_height_m);
   vertices[2] = b2Vec2(element_width_m * _element_count, element_height_m);
   vertices[3] = b2Vec2(element_width_m * _element_count, 0);

   polygon_shape.Set(vertices, 4);

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

std::vector<std::shared_ptr<GameMechanism>> MovingPlatform::load(GameNode* parent, const GameDeserializeData& data)
{
   if (!data._tmx_layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return {};
   }

   if (!data._tmx_tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> moving_platforms;
   const auto tilesize = sf::Vector2u(data._tmx_tileset->_tile_width_px, data._tmx_tileset->_tile_height_px);
   const auto tiles = data._tmx_layer->_data;
   const auto width = data._tmx_layer->_width_tl;
   const auto height = data._tmx_layer->_height_tl;
   const auto first_id = data._tmx_tileset->_first_gid;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tile_number = tiles[x + y * width];

         if (tile_number != 0)
         {
            // find matching platform
            auto moving_platform = std::make_shared<MovingPlatform>(parent);
            moving_platform->setObjectId(data._tmx_layer->_name);

            const auto texture_path = data._base_path / data._tmx_tileset->_image->_source;
            const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
            const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);

            if (std::filesystem::exists(normal_map_path))
            {
               moving_platform->_normal_map = TexturePool::getInstance().get(normal_map_path);
            }

            moving_platform->_texture_map = TexturePool::getInstance().get(texture_path);
            moving_platform->_tile_positions.x = x;
            moving_platform->_tile_positions.y = y;

            if (data._tmx_layer->_properties)
            {
               moving_platform->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
            }

            moving_platforms.push_back(moving_platform);

            while (tile_number != 0)
            {
               const auto tile_id = tile_number - first_id;
               const auto tu = (tile_id) % (moving_platform->_texture_map->getSize().x / tilesize.x);
               const auto tv = (tile_id) / (moving_platform->_texture_map->getSize().x / tilesize.x);

               sf::Sprite sprite;
               sprite.setTexture(*moving_platform->_texture_map);
               sprite.setTextureRect(sf::IntRect(tu * PIXELS_PER_TILE, tv * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE));

               sprite.setPosition(sf::Vector2f(static_cast<float_t>(x * PIXELS_PER_TILE), static_cast<float_t>(y * PIXELS_PER_TILE)));

               moving_platform->addSprite(sprite);
               moving_platform->_element_count++;

               // jump to next tile
               x++;
               tile_number = tiles[x + y * width];
            }

            moving_platform->setupBody(data._world);
            moving_platform->setupTransform();
         }
      }
   }

   return moving_platforms;
}

namespace
{
std::vector<std::shared_ptr<TmxObject>> boxes;
std::vector<std::shared_ptr<TmxObject>> paths;
}  // namespace

void MovingPlatform::deserialize(const std::shared_ptr<TmxObject>& tmx_object)
{
   // just collect all the tmx objects
   if (tmx_object->_polyline)
   {
      paths.push_back(tmx_object);
   }
   else
   {
      boxes.push_back(tmx_object);
   }
}

std::vector<std::shared_ptr<GameMechanism>> MovingPlatform::merge(GameNode* parent, const GameDeserializeData& data)
{
   std::vector<std::shared_ptr<GameMechanism>> moving_platforms;

   if (boxes.empty() || paths.empty())
   {
      return moving_platforms;
   }

   // generate pairs of matching box and poly tmx objects
   std::vector<std::pair<std::shared_ptr<TmxObject>, std::shared_ptr<TmxObject>>> box_path_pairs;

   for (const auto& box : boxes)
   {
      //      platform path
      //
      //           q1
      //           |
      // p0        |         p3
      //  +--------+--------+
      //  |        |        |
      //  |        |        | platform rect
      //  |        |        |
      //  +--------|--------+
      // p1        |        p2
      //           |
      //           |
      //           q0

      const auto p0 = sf::Vector2f{box->_x_px, box->_y_px};
      const auto p1 = sf::Vector2f{box->_x_px, box->_y_px + box->_height_px};
      const auto p2 = sf::Vector2f{box->_x_px + box->_width_px, box->_y_px + box->_height_px};
      const auto p3 = sf::Vector2f{box->_x_px + box->_width_px, box->_y_px};

      for (const auto& path : paths)
      {
         const auto& poly = path->_polyline;

         for (auto i = 0; i < poly->_polyline.size() - 1; i++)
         {
            const auto q0 = sf::Vector2f{path->_x_px, path->_y_px} + poly->_polyline[i];
            const auto q1 = sf::Vector2f{path->_x_px, path->_y_px} + poly->_polyline[i + 1];

            if (SfmlMath::intersect(p0, p1, q0, q1).has_value() || SfmlMath::intersect(p1, p2, q0, q1).has_value() ||
                SfmlMath::intersect(p2, p3, q0, q1).has_value() || SfmlMath::intersect(p3, p0, q0, q1).has_value())
            {
               box_path_pairs.emplace_back(box, path);
               break;
            }
         }
      }
   }

   // set up platforms
   const auto texture_path = data._base_path / "tilesets" / "platforms.png";
   const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
   const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);

   for (const auto& pair : box_path_pairs)
   {
      const auto& box = pair.first;
      const auto& path = pair.second;

      auto moving_platform = std::make_shared<MovingPlatform>(parent);
      moving_platforms.push_back(moving_platform);

      moving_platform->_element_count = static_cast<int32_t>(box->_width_px / PIXELS_PER_TILE);
      moving_platform->_z_index = 10;

      // load textures
      if (std::filesystem::exists(normal_map_path))
      {
         moving_platform->_normal_map = TexturePool::getInstance().get(normal_map_path);
      }

      moving_platform->_texture_map = TexturePool::getInstance().get(texture_path);

      const auto width_px = static_cast<int32_t>(box->_width_px);
      const auto width_tl = static_cast<int32_t>(width_px / PIXELS_PER_TILE);

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

      if (width_tl % 2 != 0)
      {
         moving_platform->_animated_tile_index_0 = ((width_tl + 1) / 2) - 1;
      }
      else
      {
         moving_platform->_animated_tile_index_0 = (width_tl / 2) - 1;
         moving_platform->_animated_tile_index_1 = (width_tl / 2);
      }

      for (auto i = 0; i < width_tl; i++)
      {
         auto tu_tl = 0;
         constexpr auto tv_tl = 0;

         if (width_tl > 2)
         {
            if (i == 0)  // first tile
            {
               tu_tl = 4;
            }
            else if (i == width_tl - 1)  // last tile
            {
               tu_tl = 7;
            }
            else  // other tiles
            {
               tu_tl = 5 + std::rand() % 1;  // or 6
            }
         }
         else if (width_tl == 2)
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

         sf::Sprite sprite;
         sprite.setTexture(*moving_platform->_texture_map);
         sprite.setTextureRect(sf::IntRect(
            tu_tl * PIXELS_PER_TILE,
            tv_tl * PIXELS_PER_TILE,
            PIXELS_PER_TILE,
            PIXELS_PER_TILE * 2  // 1 platform tile and one background tile for perspective
         ));

         moving_platform->addSprite(sprite);
      }

      b2Vec2 platform_pos_m;
      auto i = 0;

      auto platform_x_min = std::numeric_limits<float>::max();
      auto platform_y_min = std::numeric_limits<float>::max();
      auto platform_x_max = std::numeric_limits<float>::min();
      auto platform_y_max = std::numeric_limits<float>::min();

      for (const auto& poly_pos_px : path->_polyline->_polyline)
      {
         auto time = i / static_cast<float>(path->_polyline->_polyline.size() - 1);

         // we don't want to position the platform right on the path, we only
         // want to move its center there
         const auto x_px = (path->_x_px + poly_pos_px.x) - (moving_platform->_element_count * PIXELS_PER_TILE / 2.0f);
         const auto y_px = (path->_y_px + poly_pos_px.y);  // -     (moving_platform->_height_tl * PIXELS_PER_TILE) / 2.0f) * MPP;

         platform_pos_m.x = x_px * MPP;
         platform_pos_m.y = y_px * MPP;

         moving_platform->_interpolation.addKey(platform_pos_m, time);
         moving_platform->_pixel_path.emplace_back((path->_x_px + poly_pos_px.x), (path->_y_px + poly_pos_px.y));

         platform_x_min = std::min(platform_pos_m.x, platform_x_min);
         platform_y_min = std::min(platform_pos_m.y, platform_y_min);
         platform_x_max = std::max(platform_pos_m.x, platform_x_min);
         platform_y_max = std::max(platform_pos_m.y, platform_y_min);

         i++;
      }

      moving_platform->setupBody(data._world);
      moving_platform->_body->SetTransform(platform_pos_m, 0.0f);

      // set up bounding rect
      moving_platform->_rect.left = platform_x_min;
      moving_platform->_rect.top = platform_y_min;
      moving_platform->_rect.width = platform_x_max - platform_x_min;
      moving_platform->_rect.height = platform_y_max - platform_y_min;

      moving_platform->addChunks(moving_platform->_rect);
   }

   // clean up
   paths.clear();
   boxes.clear();

   return moving_platforms;
}

void MovingPlatform::link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, const GameDeserializeData& data)
{
   if (!data._tmx_object->_polyline)
   {
      return;
   }

   std::vector<sf::Vector2f> pixel_path = data._tmx_object->_polyline->_polyline;

   const auto pos = pixel_path.at(0);
   const auto x_tl = static_cast<int>((pos.x + data._tmx_object->_x_px) / PIXELS_PER_TILE);
   const auto y_tl = static_cast<int>((pos.y + data._tmx_object->_y_px) / PIXELS_PER_TILE);

   std::shared_ptr<MovingPlatform> platform;

   for (const auto& p : platforms)
   {
      auto tmp = std::dynamic_pointer_cast<MovingPlatform>(p);
      if (tmp->_tile_positions.y == y_tl)
      {
         for (auto xi = 0; xi < tmp->_element_count; xi++)
         {
            if (tmp->_tile_positions.x + xi == x_tl)
            {
               platform = tmp;
               // printf("linking tmx poly to platform at %d, %d\n", x, y);
               break;
            }
         }
      }

      // we're done when we found a matching platform
      if (platform)
      {
         break;
      }
   }

   if (platform)
   {
      auto i = 0;
      for (const auto& poly_pos : pixel_path)
      {
         b2Vec2 platform_pos;
         const auto time = i / static_cast<float>(pixel_path.size() - 1);

         // where do those 4px error come from?!
         const auto x = (data._tmx_object->_x_px + poly_pos.x - 4 - (platform->_element_count * PIXELS_PER_TILE) / 2.0f) * MPP;
         const auto y = (data._tmx_object->_y_px + poly_pos.y - (PIXELS_PER_TILE) / 2.0f) * MPP;

         platform_pos.x = x;
         platform_pos.y = y;

         platform->_interpolation.addKey(platform_pos, time);
         platform->_pixel_path.emplace_back((pos.x + data._tmx_object->_x_px), (pos.y + data._tmx_object->_y_px));

         i++;
      }
   }
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

void MovingPlatform::updateLeverLag(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (_lever_lag <= 0.0f)
      {
         _lever_lag = 0.0f;
      }
      else
      {
         _lever_lag -= dt.asSeconds();
      }
   }
   else
   {
      if (_lever_lag < 1.0f)
      {
         _lever_lag += dt.asSeconds();
      }
      else
      {
         _lever_lag = 1.0f;
      }
   }
}

void MovingPlatform::update(const sf::Time& dt)
{
   updateLeverLag(dt);
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
   auto horizontal = (_element_count > 1) ? 1 : 0;

   constexpr auto animation_tile_count = 4;
   constexpr auto animation_speed_factor = 10.0f;
   _animation_elapsed += _lever_lag * dt.asSeconds() * animation_speed_factor;
   const auto animation_tile_index = static_cast<int32_t>(_animation_elapsed) % animation_tile_count;

   for (auto& sprite : _sprites)
   {
      const auto x = _body->GetPosition().x * PPM + horizontal * sprite_index * PIXELS_PER_TILE;
      const auto y = _body->GetPosition().y * PPM - PIXELS_PER_TILE;  // there's one tile offset for the perspective tile

      sprite.setPosition(x, y);
      auto update_sprite_rect = false;
      auto u = 0;
      auto v = 0;

      if (_sprites.size() == 2)
      {
         update_sprite_rect = true;
         u = (animation_tile_index * 2 + sprite_index) * PIXELS_PER_TILE;
         v = PIXELS_PER_TILE * 4;
      }
      else if (_sprites.size() > 2)
      {
         if (_sprites.size() % 2 == 0)  // handle even tile counts
         {
            if (sprite_index == ((_sprites.size() + 1) / 2) - 1)
            {
               update_sprite_rect = true;
               u = (animation_tile_index * 2) * PIXELS_PER_TILE;
               v = PIXELS_PER_TILE * 2;
            }
            else if (sprite_index == ((_sprites.size() + 1) / 2))
            {
               update_sprite_rect = true;
               u = (animation_tile_index * 2 + 1) * PIXELS_PER_TILE;
               v = PIXELS_PER_TILE * 2;
            }
         }
         else  // handle uneven tile counts
         {
            if (sprite_index == ((_sprites.size() + 1) / 2) - 1)
            {
               update_sprite_rect = true;
               u = animation_tile_index * PIXELS_PER_TILE;
               v = 0;
            }
         }
      }

      if (update_sprite_rect)
      {
         sprite.setTextureRect({u, v, PIXELS_PER_TILE, PIXELS_PER_TILE * 2});
      }

      sprite_index++;
   }
}

std::optional<sf::FloatRect> MovingPlatform::getBoundingBoxPx()
{
   return _rect;
}
