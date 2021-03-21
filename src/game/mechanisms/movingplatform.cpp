// base
#include "movingplatform.h"

#include "constants.h"
#include "fixturenode.h"
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
   // std::cout << mInitialized << std::endl;

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
void MovingPlatform::setOffset(float x, float y)
{
   _x = x;
   _y = y;
}


//-----------------------------------------------------------------------------
void MovingPlatform::setupTransform()
{
   auto x = _tile_positions.x * PIXELS_PER_TILE / PPM;
   auto y = _tile_positions.y * PIXELS_PER_TILE / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void MovingPlatform::setupBody(const std::shared_ptr<b2World>& world)
{
   b2PolygonShape polygon_shape;
   auto size_x = PIXELS_PER_TILE / PPM;
   auto size_y = 0.5f * PIXELS_PER_TILE / PPM;
   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(0,               0               );
   vertices[1] = b2Vec2(0,               size_y * _height);
   vertices[2] = b2Vec2(size_x * _width, size_y * _height);
   vertices[3] = b2Vec2(size_x * _width, 0               );
   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   setupTransform();

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
   std::vector<std::shared_ptr<GameMechanism>> movingPlatforms;
   const auto tilesize = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

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

            const auto texture_path = basePath / tileSet->mImage->mSource;
            const auto normal_map_filename = (texture_path.stem().string() + "_normals" + texture_path.extension().string());
            const auto normal_map_path = (texture_path.parent_path() / normal_map_filename);

            moving_platform->_texture_map = TexturePool::getInstance().get(texture_path);
            moving_platform->_normal_map = TexturePool::getInstance().get(normal_map_path);
            moving_platform->_tile_positions.x = x;
            moving_platform->_tile_positions.y = y;

            if (layer->mProperties != nullptr)
            {
               moving_platform->setZ(layer->mProperties->mMap["z"]->mValueInt.value());
            }

            movingPlatforms.push_back(moving_platform);

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
               moving_platform->_width++;

               // jump to next tile
               x++;
               tile_number = tiles[x + y * width];
            }

            moving_platform->setupBody(world);

            // printf(
            //   "created MovingPlatform %zd at %d, %d (width: %zd)\n",
            //   movingPlatforms.size(),
            //   x,
            //   y,
            //   movingPlatform->mSprites.size()
            // );
         }
      }
   }

   return movingPlatforms;
}


//-----------------------------------------------------------------------------
void MovingPlatform::link(const std::vector<std::shared_ptr<GameMechanism>>& platforms, TmxObject *tmx_object)
{
   std::vector<sf::Vector2f> pixel_path = tmx_object->mPolyLine->mPolyLine;

   auto pos = pixel_path.at(0);

   auto x = static_cast<int>((pos.x + tmx_object->mX) / PIXELS_PER_TILE);
   auto y = static_cast<int>((pos.y + tmx_object->mY) / PIXELS_PER_TILE);

   std::shared_ptr<MovingPlatform> platform;

   for (auto& p : platforms)
   {
      auto tmp = std::dynamic_pointer_cast<MovingPlatform>(p);
      if (tmp->_tile_positions.y == y)
      {
         for (auto xi = 0; xi < tmp->_width; xi++)
         {
            if (tmp->_tile_positions.x + xi == x)
            {
               platform = tmp;
               // printf("linking tmx poly to platform at %d, %d\n", x, y);
               break;
            }
         }
      }

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
         auto x = (tmx_object->mX + poly_pos.x - 4 - (platform->_width  * PIXELS_PER_TILE) / 2.0f) * MPP;
         auto y = (tmx_object->mY + poly_pos.y -     (platform->_height * PIXELS_PER_TILE) / 2.0f) * MPP;

         platform_pos.x = x;
         platform_pos.y = y;

         platform->_interpolation.addKey(platform_pos, time);
         platform->_pixel_path.push_back({(pos.x + tmx_object->mX), (pos.y + tmx_object->mY)});

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
   auto horizontal = (_width  > 1) ? 1 : 0;
   auto vertical   = (_height > 1) ? 1 : 0;

   for (auto& sprite : _sprites)
   {
      auto x = _body->GetPosition().x * PPM + horizontal * pos * PIXELS_PER_TILE;
      auto y = _body->GetPosition().y * PPM + vertical   * pos * PIXELS_PER_TILE;

      sprite.setPosition(x, y);

      pos++;
   }
}

