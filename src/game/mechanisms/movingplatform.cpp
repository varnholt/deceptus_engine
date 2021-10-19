// base
#include "movingplatform.h"

#include "constants.h"
#include "fixturenode.h"
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/globalclock.h"
#include "level.h"
#include "player/player.h"
#include "physics/physicsconfiguration.h"
#include "texturepool.h"

#include <iostream>
#include <math.h>

#include "Box2D/Box2D.h"

namespace
{
constexpr auto element_width_m  =        PIXELS_PER_TILE / PPM;
constexpr auto element_height_m = 0.5f * PIXELS_PER_TILE / PPM;
}


//-----------------------------------------------------------------------------
MovingPlatform::MovingPlatform(GameNode *parent)
 : GameNode(parent)
{
   setName(typeid(MovingPlatform).name());
}


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
double MovingPlatform::CosineInterpolate(double y1, double y2, double mu)
{
   double mu2 = (1.0 - cos(mu * M_PI)) * 0.5;
   return (y1 * (1.0 - mu2) + y2 * mu2);
}


//-----------------------------------------------------------------------------
const std::vector<sf::Vector2f>& MovingPlatform::getPixelPath() const
{
   return _pixel_path;
}


//-----------------------------------------------------------------------------
const PathInterpolation& MovingPlatform::getInterpolation() const
{
   return _interpolation;
}


//-----------------------------------------------------------------------------
b2Body* MovingPlatform::getBody()
{
   return _body;
}


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
void MovingPlatform::setupTransformDeprecated()
{
   auto x = _tile_positions.x * PIXELS_PER_TILE / PPM;
   auto y = _tile_positions.y * PIXELS_PER_TILE / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void MovingPlatform::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,                                0               );
   vertices[1] = b2Vec2(0,                                element_height_m);
   vertices[2] = b2Vec2(element_width_m * _element_count, element_height_m);
   vertices[3] = b2Vec2(element_width_m * _element_count, 0               );

   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   auto fixture = _body->CreateFixture(&polygon_shape, 0);
   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMovingPlatform);
   fixture->SetUserData(static_cast<void*>(object_data));
}


//-----------------------------------------------------------------------------
void MovingPlatform::addSprite(const sf::Sprite& sprite)
{
   _sprites.push_back(sprite);
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> MovingPlatform::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& world
)
{
   std::vector<std::shared_ptr<GameMechanism>> moving_platforms;
   const auto tilesize = sf::Vector2u(tileSet->_tile_width_px, tileSet->_tile_height_px);
   const auto tiles    = layer->_data;
   const auto width    = layer->_width_px;
   const auto height   = layer->_height_px;
   const auto firstId  = tileSet->_first_gid;

   for (auto y = 0u; y < height; y++)
   {
      for (auto x = 0u; x < width; x++)
      {
         // get the current tile number
         auto tile_number = tiles[x + y * width];

         if (tile_number != 0)
         {
            // find matching platform
            auto moving_platform = std::make_shared<MovingPlatform>(Level::getCurrentLevel());

            const auto texture_path = basePath / tileSet->_image->_source;
            const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
            const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);

            if (std::filesystem::exists(normal_map_path))
            {
               moving_platform->_normal_map = TexturePool::getInstance().get(normal_map_path);
            }

            moving_platform->_texture_map = TexturePool::getInstance().get(texture_path);
            moving_platform->_tile_positions.x = x;
            moving_platform->_tile_positions.y = y;

            if (layer->_properties != nullptr)
            {
               moving_platform->setZ(layer->_properties->_map["z"]->_value_int.value());
            }

            moving_platforms.push_back(moving_platform);

            while (tile_number != 0)
            {
               auto tileId = tile_number - firstId;
               auto tu = (tileId) % (moving_platform->_texture_map->getSize().x / tilesize.x);
               auto tv = (tileId) / (moving_platform->_texture_map->getSize().x / tilesize.x);

               sf::Sprite sprite;
               sprite.setTexture(*moving_platform->_texture_map);
               sprite.setTextureRect(
                  sf::IntRect(
                     tu * PIXELS_PER_TILE,
                     tv * PIXELS_PER_TILE,
                     PIXELS_PER_TILE,
                     PIXELS_PER_TILE
                  )
               );

               sprite.setPosition(
                  sf::Vector2f(
                     static_cast<float_t>(x * PIXELS_PER_TILE),
                     static_cast<float_t>(y * PIXELS_PER_TILE)
                  )
               );

               moving_platform->addSprite(sprite);
               moving_platform->_element_count++;

               // jump to next tile
               x++;
               tile_number = tiles[x + y * width];
            }

            moving_platform->setupBody(world);
            moving_platform->setupTransformDeprecated();
         }
      }
   }

   return moving_platforms;
}


namespace
{
std::vector<TmxObject*> boxes;
std::vector<TmxObject*> paths;
}



//-----------------------------------------------------------------------------
void MovingPlatform::deserialize(TmxObject* tmx_object)
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


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> MovingPlatform::merge(
   const std::filesystem::path& base_path,
   const std::shared_ptr<b2World>& world
)
{
   std::vector<std::shared_ptr<GameMechanism>> moving_platforms;

   if (boxes.empty() || paths.empty())
   {
      return moving_platforms;
   }

   // generate pairs of matching box and poly tmx objects
   std::vector<std::pair<TmxObject*, TmxObject*>> box_path_pairs;

   for (auto box : boxes)
   {

      //      platform path
      //
      //           q1
      //           |
      //           |
      //  +--------+--------+
      //  |        |        |
      //  |        |        | platform rect
      //  |        |        |
      //  +--------|--------+
      // p0        |        p1
      //           |
      //           |
      //           q0


      auto p0 = sf::Vector2f{box->_x_px,                  box->_y_px};
      auto p1 = sf::Vector2f{box->_x_px + box->_width_px, box->_y_px};

      for (auto path : paths)
      {
         auto poly = path->_polyline;

         for (auto i = 0; i < poly->_polyline.size() - 1; i++)
         {
            auto q0 = sf::Vector2f{path->_x_px, path->_y_px} + poly->_polyline[i];
            auto q1 = sf::Vector2f{path->_x_px, path->_y_px} + poly->_polyline[i + 1];

            if (SfmlMath::intersect(p0, p1, q0, q1).has_value())
            {
               box_path_pairs.push_back({box, path});
               break;
            }
         }
      }
   }

   // set up platforms
   const auto texture_path = base_path / "tilesets" / "platforms.png";
   const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
   const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);

   for (const auto& pair : box_path_pairs)
   {
      auto box = pair.first;
      auto path = pair.second;

      auto moving_platform = std::make_shared<MovingPlatform>(Level::getCurrentLevel());
      moving_platforms.push_back(moving_platform);

      moving_platform->_element_count = static_cast<int32_t>(box->_width_px / PIXELS_PER_TILE);
      moving_platform->_z_index = 10;

      // load textures
      if (std::filesystem::exists(normal_map_path))
      {
         moving_platform->_normal_map = TexturePool::getInstance().get(normal_map_path);
      }

      moving_platform->_texture_map = TexturePool::getInstance().get(texture_path);

      auto width_px = box->_width_px;
      auto width_tl = width_px / PIXELS_PER_TILE;

      for (auto i = 0; i < width_tl; i++)
      {
         // 0123 4567
         // anim        -> wheel animation
         //      lmmr   -> regular parts
         //
         // 01 23 45 67 -> 2 tile animation
         // a0 a1 a2 a3
         auto tu_tl = 0;
         auto tv_tl = 0;

         if (width_tl == 3)
         {
            if (i == 0) // first tile
            {
               tu_tl = 4;
            }
            else if (i == width_tl - 1) // last tile
            {
               tu_tl = 7;
            }
            else // other tiles
            {
               tu_tl = 5; // or 6
            }

            // handle middle tile
            // ...
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
         sprite.setTextureRect(
            sf::IntRect(
               tu_tl * PIXELS_PER_TILE,
               tv_tl * PIXELS_PER_TILE,
               PIXELS_PER_TILE,
               PIXELS_PER_TILE * 2  // 1 platform tile and one background tile for perspective
            )
         );

         moving_platform->addSprite(sprite);
      }

      b2Vec2 platform_pos_m;
      auto i = 0;
      for (const auto& poly_pos_px : path->_polyline->_polyline)
      {
         auto time = i / static_cast<float>(path->_polyline->_polyline.size() - 1);

         // we don't want to position the platform right on the path, we only
         // want to move its center there
         auto x_px = (path->_x_px + poly_pos_px.x) - (moving_platform->_element_count * PIXELS_PER_TILE / 2.0f);
         auto y_px = (path->_y_px + poly_pos_px.y); // -     (moving_platform->_height_tl * PIXELS_PER_TILE) / 2.0f) * MPP;

         platform_pos_m.x = x_px * MPP;
         platform_pos_m.y = y_px * MPP;

         moving_platform->_interpolation.addKey(platform_pos_m, time);
         moving_platform->_pixel_path.push_back({(path->_x_px + poly_pos_px.x), (path->_y_px + poly_pos_px.y)});

         i++;
      }

      moving_platform->setupBody(world);
      moving_platform->_body->SetTransform(platform_pos_m, 0.0f);
   }

   // clean up
   paths.clear();
   boxes.clear();

   return moving_platforms;
}


//-----------------------------------------------------------------------------
void MovingPlatform::link(
   const std::vector<std::shared_ptr<GameMechanism>>& platforms,
   TmxObject* tmx_object
)
{
   std::vector<sf::Vector2f> pixel_path = tmx_object->_polyline->_polyline;

   auto pos = pixel_path.at(0);

   auto x = static_cast<int>((pos.x + tmx_object->_x_px) / PIXELS_PER_TILE);
   auto y = static_cast<int>((pos.y + tmx_object->_y_px) / PIXELS_PER_TILE);

   std::shared_ptr<MovingPlatform> platform;

   for (auto& p : platforms)
   {
      auto tmp = std::dynamic_pointer_cast<MovingPlatform>(p);
      if (tmp->_tile_positions.y == y)
      {
         for (auto xi = 0; xi < tmp->_element_count; xi++)
         {
            if (tmp->_tile_positions.x + xi == x)
            {
               platform = tmp;
               // printf("linking tmx poly to platform at %d, %d\n", x, y);
               break;
            }
         }
      }

      // we're done when we found a matching platform
      if (platform != nullptr)
      {
         break;
      }
   }

   if (platform != nullptr)
   {
      auto i = 0;
      for (const auto& poly_pos : pixel_path)
      {
         b2Vec2 platform_pos;
         auto time = i / static_cast<float>(pixel_path.size() - 1);

         // where do those 4px error come from?!
         auto x = (tmx_object->_x_px + poly_pos.x - 4 - (platform->_element_count  * PIXELS_PER_TILE) / 2.0f) * MPP;
         auto y = (tmx_object->_y_px + poly_pos.y -                                 (PIXELS_PER_TILE) / 2.0f) * MPP;

         platform_pos.x = x;
         platform_pos.y = y;

         platform->_interpolation.addKey(platform_pos, time);
         platform->_pixel_path.push_back({(pos.x + tmx_object->_x_px), (pos.y + tmx_object->_y_px)});

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


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
void MovingPlatform::update(const sf::Time& dt)
{
   updateLeverLag(dt);
   _interpolation.update(_body->GetPosition());
    _body->SetLinearVelocity(_lever_lag * TIMESTEP_ERROR * (PPM / 60.0f) * _interpolation.getVelocity());

   auto pos = 0;
   auto horizontal = (_element_count  > 1) ? 1 : 0;

   for (auto& sprite : _sprites)
   {
      auto x = _body->GetPosition().x * PPM + horizontal * pos * PIXELS_PER_TILE;
      auto y = _body->GetPosition().y * PPM - PIXELS_PER_TILE; // there's one tile offset for the perspective tile

      sprite.setPosition(x, y);

      pos++;
   }
}

