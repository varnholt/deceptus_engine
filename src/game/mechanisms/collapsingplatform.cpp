#include "CollapsingPlatform.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include "framework/tools/globalclock.h"


namespace
{
static constexpr auto bevel_m = 4 * MPP;

static constexpr auto columns = 12;
static constexpr auto tiles_per_box_width = 4;
static constexpr auto tiles_per_box_height = 3;

static constexpr auto animation_speed = 8.0f;

static constexpr auto sprite_offset_x_px = 0;
static constexpr auto sprite_offset_y_px = 0;
}


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

   const auto width_m  = data._tmx_object->_width_px * MPP;
   const auto height_m = data._tmx_object->_height_px * MPP;

   std::array<b2Vec2, 6> vertices {
      b2Vec2{bevel_m,             0.0f    },
      b2Vec2{0.0f,                bevel_m },
      b2Vec2{0.0f,                height_m},
      b2Vec2{width_m,             height_m},
      b2Vec2{width_m,             bevel_m },
      b2Vec2{width_m - bevel_m,   0.0f    },
   };

   _shape.Set(vertices.data(), static_cast<int32_t>(vertices.size()));

   // create body
   const auto x = data._tmx_object->_x_px;
   const auto y = data._tmx_object->_y_px;
   _position_m = MPP * b2Vec2{x, y};

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
   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "collapsing_platform.png");
   _sprite.setTexture(*_texture);
   _sprite.setPosition(x + sprite_offset_x_px, y + sprite_offset_y_px);

   _sprite.setTextureRect({
         0,
         0,
         static_cast<int32_t>(data._tmx_object->_width_px),
         static_cast<int32_t>(data._tmx_object->_height_px)
      }
   );
}


void CollapsingPlatform::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   auto sprite_index = 0;

   if (_collapsed)
   {
      sprite_index = std::min(static_cast<int32_t>(_collapse_elapsed_s * animation_speed), columns - 1);
   }
   else
   {
      sprite_index = 0;
   }

//   _sprite.setTextureRect({
//         sprite_index * PIXELS_PER_TILE * tiles_per_box_width,
//         (_collapsed ? 1 : 0) * PIXELS_PER_TILE * tiles_per_box_height,
//         PIXELS_PER_TILE * tiles_per_box_width,
//         PIXELS_PER_TILE * tiles_per_box_height
//      }
//   );

   color.draw(_sprite);
}


void CollapsingPlatform::update(const sf::Time& dt)
{
   _elapsed_s += dt.asSeconds();
   _collapse_elapsed_s += dt.asSeconds();

   if (_collapsed)
   {
      _body->SetActive(false);
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

   // measure the time elapsed on the platform instead

//   if (_contact_count == 0)
//   {
//      _collapsed = true;
//      _collapse_time = GlobalClock::getInstance().getElapsedTime();
//      _pop_elapsed_s = 0.0f;
//   }
}


