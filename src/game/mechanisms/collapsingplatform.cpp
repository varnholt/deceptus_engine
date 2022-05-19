#include "CollapsingPlatform.h"

#include "constants.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "texturepool.h"

#include "framework/tools/globalclock.h"


namespace
{
constexpr auto sprite_column_count = 38;
constexpr auto bevel_m = 4 * MPP;
constexpr auto animation_speed = 8.0f;
constexpr auto collapse_time_s = 3.0f;
constexpr auto sprite_offset_y_px = -24;
constexpr auto destruction_speed = 30.0f;
constexpr auto fall_speed = 6.0f;
}

// animation info
//
// row 0:    trigger state
// row 1:    appear state
// row 2-6:  collapse state


CollapsingPlatform::CollapsingPlatform(
   GameNode* parent,
   const GameDeserializeData& data
)
 : FixtureNode(parent)
{
   setClassName(typeid(CollapsingPlatform).name());
   setType(ObjectTypeCollapsingPlatform);

   // read properties
   if (data._tmx_object->_properties)
   {
   }

   // set up shape
   //
   //       0        5
   //       +--------+
   //      /          \
   //   1 +            + 4
   //     |            |
   //   2 +------------+ 3

   const auto _width_m  = data._tmx_object->_width_px * MPP;
   const auto _height_m = data._tmx_object->_height_px * MPP;

   if (_width_m < 0.01f || _height_m < 0.01f)
   {
      Log::Error() << "collapsing platform has invalid dimensions, object id: " << data._tmx_object->_id;
      return;
   }

   _width_tl = static_cast<int32_t>(data._tmx_object->_width_px / PIXELS_PER_TILE);

   _blocks.resize(_width_tl);

   std::array<b2Vec2, 6> vertices {
      b2Vec2{bevel_m,              0.0f    },
      b2Vec2{0.0f,                 bevel_m },
      b2Vec2{0.0f,                 _height_m},
      b2Vec2{_width_m,             _height_m},
      b2Vec2{_width_m,             bevel_m },
      b2Vec2{_width_m - bevel_m,   0.0f    },
   };

   _shape.CreateLoop(vertices.data(), static_cast<int32_t>(vertices.size()));

   // create body
   const auto x = data._tmx_object->_x_px;
   const auto y = data._tmx_object->_y_px;
   _position_m = MPP * b2Vec2{x, y};
   _position_px = sf::Vector2f(x, y);

   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = _position_m;
   _body = data._world->CreateBody(&body_def);

   // set up body fixture
   b2FixtureDef fixture_def;
   fixture_def.shape = &_shape;
   fixture_def.density = 1.0f;
   fixture_def.isSensor = false;
   _fixture = _body->CreateFixture(&fixture_def);
   _fixture->SetUserData(static_cast<void*>(this));

   // set up visualization
   _texture = TexturePool::getInstance().get("data/sprites/collapsing_platform.png");

   // initialize all blocks
   auto sprite_offset_x_px = 0;
   auto row_index = 0;
   for (auto& block : _blocks)
   {
      block._sprite.setTexture(*_texture);
      block._x_px = x + sprite_offset_x_px;
      block._y_px = y + sprite_offset_y_px;
      block._sprite_row = row_index % 4;
      block._fall_speed = 1.0f + (std::rand() % 256) / 256.0f;
      block._destruction_speed = 1.0f + (std::rand() % 256) / 256.0f;

      sprite_offset_x_px += PIXELS_PER_TILE;
      row_index++;
   }
}


void CollapsingPlatform::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   updateBlock();

   if (_blocks.empty() || _blocks[0]._sprite_column == sprite_column_count)
   {
      return;
   }

   for (auto& block: _blocks)
   {
      color.draw(block._sprite);
   }
}


void CollapsingPlatform::update(const sf::Time& dt)
{
   if (!_body)
   {
      return;
   }

   _elapsed_s += dt.asSeconds();

   // measure the time elapsed on the platform instead
   if (_contact_count > 0)
   {
      _collapse_elapsed_s += dt.asSeconds();

      if (_collapse_elapsed_s > collapse_time_s)
      {
         if (!_collapsed)
         {
            _collapsed = true;
            _body->SetActive(false);
         }
      }
   }
   else
   {
      if (!_collapsed)
      {
         _body->SetActive(true);
         _collapse_elapsed_s = 0.0f;
      }
   }

   if (_collapsed)
   {
      const auto dt_s = dt.asSeconds();;
      for (auto& block : _blocks)
      {
         block._elapsed_s += dt_s;
         block._sprite_column = std::min(static_cast<int32_t>(block._elapsed_s * destruction_speed * block._destruction_speed), sprite_column_count);
         block._fall_offset_y_px = block._elapsed_s * block._fall_speed * fall_speed;
      }
   }
}


void CollapsingPlatform::beginContact()
{
   if (_collapsed)
   {
      return;
   }

   _contact_count++;
}


void CollapsingPlatform::endContact()
{
   if (_collapsed)
   {
      return;
   }

   _contact_count--;
}


void CollapsingPlatform::updateBlock()
{
   for (auto& block : _blocks)
   {
      block._sprite.setPosition(block._x_px, block._y_px + block._fall_offset_y_px);
      block._sprite.setTextureRect({
            block._sprite_column * PIXELS_PER_TILE,
            block._sprite_row * PIXELS_PER_TILE * 3,
            PIXELS_PER_TILE,
            PIXELS_PER_TILE * 3
         }
      );
   }
}


