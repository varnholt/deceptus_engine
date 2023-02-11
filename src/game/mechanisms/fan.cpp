#include "fan.h"

#include "constants.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"

#include "texturepool.h"

#include <array>
#include <iostream>

std::vector<std::shared_ptr<GameMechanism>> Fan::__fan_instances;
std::vector<std::shared_ptr<Fan::FanTile>> Fan::__tile_instances;
std::vector<std::shared_ptr<TmxObject>> Fan::__object_instances;
std::vector<sf::Vector2f> Fan::__weight_instances;

Fan::Fan(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Fan).name());
}

void Fan::createPhysics(const std::shared_ptr<b2World>& world, const std::shared_ptr<FanTile>& tile)
{
   auto possf = tile->_position;
   auto posb2d = b2Vec2(possf.x * MPP, possf.y * MPP);

   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = posb2d;
   tile->_body = world->CreateBody(&body_def);

   // create fixture for physical boundaries of the fan object
   b2PolygonShape shape;

   // a rounded box prevents the player of getting stuck between the gaps
   //
   //      h        g
   //      _________
   //     /         \
   //   a |         | f
   //     |         |
   //   b |         | e
   //     \________/
   //
   //      c      d

   static constexpr float w = 0.5f;
   static constexpr float e = 0.1f;  // 219, 194
   std::array<b2Vec2, 8> rounded_box{
      b2Vec2{0, e},      // a
      b2Vec2{0, w - e},  // b
      b2Vec2{e, w},      // c
      b2Vec2{w - e, w},  // d
      b2Vec2{w, w - e},  // e
      b2Vec2{w, e},      // f
      b2Vec2{w - e, 0},  // g
      b2Vec2{e, 0},      // h
   };

   shape.Set(rounded_box.data(), static_cast<int32_t>(rounded_box.size()));

   b2FixtureDef boundary_fixture_def;
   boundary_fixture_def.shape = &shape;
   boundary_fixture_def.density = 1.0f;
   boundary_fixture_def.isSensor = false;
   tile->_body->CreateFixture(&boundary_fixture_def);
}

const sf::FloatRect& Fan::getPixelRect() const
{
   return _pixel_rect;
}

void Fan::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
   _lever_lag = enabled ? 0.0f : 1.0f;
}

std::vector<std::shared_ptr<GameMechanism>>& Fan::getFans()
{
   return __fan_instances;
}

void Fan::updateSprite()
{
   auto y_offset_tl = 0;
   const auto dir = _tiles.at(0)->_tile_dir;
   switch (dir)
   {
      case TileDirection::Up:
         y_offset_tl = 0;
         break;
      case TileDirection::Right:
         y_offset_tl = 1;
         break;
      case TileDirection::Left:
         y_offset_tl = 2;
         break;
      case TileDirection::Down:
         y_offset_tl = 3;
         break;
   }

   auto index = 0u;
   for (auto& sprite : _sprites)
   {
      auto x_offset_tl = static_cast<int32_t>(_x_offsets_px[index]) % 8;
      sprite.setTextureRect({x_offset_tl * PIXELS_PER_TILE, y_offset_tl * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});

      index++;
   }
}

void Fan::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   for (auto& sprite : _sprites)
   {
      color.draw(sprite);
   }
}

void Fan::update(const sf::Time& dt)
{
   if (!isEnabled())
   {
      if (_lever_lag <= 0.0f)
      {
         return;
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

   for (auto& x_offset : _x_offsets_px)
   {
      x_offset += dt.asSeconds() * 25.0f * _speed * _lever_lag;
   }

   updateSprite();
}

std::optional<sf::FloatRect> Fan::getBoundingBoxPx()
{
   return sf::FloatRect(_pixel_rect.left, _pixel_rect.top, _pixel_rect.width, _pixel_rect.height);
}

void Fan::load(const GameDeserializeData& data)
{
   static const sf::Vector2f vector_up{0.0f, 1.0f};
   static const sf::Vector2f vector_down{0.0f, -1.0f};
   static const sf::Vector2f vector_left{-1.0f, 0.0f};
   static const sf::Vector2f vector_right{1.0f, 0.0f};

   if (data._tmx_layer == nullptr)
   {
      return;
   }

   if (data._tmx_tileset == nullptr)
   {
      return;
   }

   resetAll();

   const auto tiles = data._tmx_layer->_data;
   const auto width = data._tmx_layer->_width_tl;
   const auto height = data._tmx_layer->_height_tl;
   const auto firstId = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         const auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            const auto direction = static_cast<TileDirection>(tile_number - firstId);
            sf::Vector2f direction_vector;

            switch (direction)
            {
               case TileDirection::Up:
                  direction_vector = vector_up;
                  break;
               case TileDirection::Left:
                  direction_vector = vector_left;
                  break;
               case TileDirection::Right:
                  direction_vector = vector_right;
                  break;
               case TileDirection::Down:
                  direction_vector = vector_down;
                  break;
            }

            auto tile = std::make_shared<FanTile>();

            const auto x = i * PIXELS_PER_TILE;
            const auto y = j * PIXELS_PER_TILE;

            tile->_position = sf::Vector2i(i * PIXELS_PER_TILE, j * PIXELS_PER_TILE);
            tile->_rect.left = static_cast<float>(x);
            tile->_rect.top = static_cast<float>(y);
            tile->_rect.width = PIXELS_PER_TILE;
            tile->_rect.height = PIXELS_PER_TILE;
            tile->_tile_dir = direction;
            tile->_direction = direction_vector;
            __tile_instances.push_back(tile);

            createPhysics(data._world, tile);
         }
      }
   }
}

void Fan::resetAll()
{
   __fan_instances.clear();
   __tile_instances.clear();
   __object_instances.clear();
   __weight_instances.clear();
}

void Fan::addObject(GameNode* parent, const GameDeserializeData& data)
{
   __object_instances.push_back(data._tmx_object);

   auto fan = std::make_shared<Fan>(parent);
   __fan_instances.push_back(fan);

   const auto w = data._tmx_object->_width_px;
   const auto h = data._tmx_object->_height_px;

   fan->_texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "fan.png");
   fan->_pixel_rect.left = data._tmx_object->_x_px;
   fan->_pixel_rect.top = data._tmx_object->_y_px;
   fan->_pixel_rect.width = w;
   fan->_pixel_rect.height = h;

   if (data._tmx_object->_properties)
   {
      auto speed_property = data._tmx_object->_properties->_map["speed"];
      fan->_speed = speed_property ? speed_property->_value_float.value() : 1.0f;
   }
}

std::optional<sf::Vector2f> Fan::collide(const sf::FloatRect& player_rect)
{
   // need to find all intersections since there can be more than one
   auto valid = false;
   sf::Vector2f dir;

   for (const auto& f : __fan_instances)
   {
      auto fan = std::dynamic_pointer_cast<Fan>(f);

      if (!fan->isEnabled())
      {
         continue;
      }

      if (player_rect.intersects(fan->_pixel_rect))
      {
         dir += fan->_direction;
         valid = true;
      }
   }

   if (valid)
   {
      return dir;
   }
   else
   {
      return {};
   }
}

void Fan::collide(const sf::FloatRect& player_rect, b2Body* body)
{
   auto dir = collide(player_rect);
   if (dir.has_value())
   {
      body->ApplyForceToCenter(b2Vec2(2.0f * dir->x, -dir->y), true);
   }
}

void Fan::merge()
{
   auto x_offset_px = 0.0f;
   for (auto& tile : __tile_instances)
   {
      for (auto& f : __fan_instances)
      {
         auto fan = std::dynamic_pointer_cast<Fan>(f);
         if (tile->_rect.intersects(fan->_pixel_rect))
         {
            sf::Sprite sprite;
            sprite.setTexture(*fan->_texture);
            sprite.setPosition(static_cast<float>(tile->_position.x), static_cast<float>(tile->_position.y));

            fan->_tiles.push_back(tile);
            fan->_direction = tile->_direction * fan->_speed;
            fan->_sprites.push_back(sprite);
            fan->_x_offsets_px.push_back(x_offset_px);
            fan->updateSprite();

            x_offset_px += 1.0f;
         }
      }
   }

   __tile_instances.clear();
}
