#include "bubblecube.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"

#include "framework/tools/globalclock.h"


namespace
{
static constexpr auto width_m  = 36 * MPP;
static constexpr auto height_m = 36 * MPP;
static constexpr auto bevel_m = 6 * MPP;

static constexpr auto columns = 12;
static constexpr auto tiles_per_box_width = 4;
static constexpr auto tiles_per_box_height = 3;

static constexpr auto animation_speed = 8.0f;
static constexpr auto move_amplitude = 0.08f;
static constexpr auto move_frequency = 4.19f;

static constexpr auto pop_frequency = 15.0f;

static constexpr auto sprite_offset_x_px = -30;
static constexpr auto sprite_offset_y_px = -14;
}


BubbleCube::BubbleCube(GameNode* parent, const GameDeserializeData& data)
 : FixtureNode(parent)
{
   setClassName(typeid(BubbleCube).name());
   setType(ObjectTypeBubbleCube);

   // read properties
   if (data._tmx_object->_properties)
   {
      const auto animation_offset_it = data._tmx_object->_properties->_map.find("animation_offset_s");
      if (animation_offset_it != data._tmx_object->_properties->_map.end())
      {
         _animation_offset_s = animation_offset_it->second->_value_float.value();
      }

      const auto pop_time_respawn_it = data._tmx_object->_properties->_map.find("pop_time_respawn_s");
      if (pop_time_respawn_it != data._tmx_object->_properties->_map.end())
      {
         _pop_time_respawn_s = pop_time_respawn_it->second->_value_float.value();
      }

      const auto pop_only_on_foot_contact_it = data._tmx_object->_properties->_map.find("pop_only_on_foot_contact");
      if (pop_only_on_foot_contact_it != data._tmx_object->_properties->_map.end())
      {
         _pop_only_on_foot_contact = pop_only_on_foot_contact_it->second->_value_bool.value();
      }

      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         setZ(static_cast<uint32_t>(z_it->second->_value_int.value()));
      }
   }

   // set up shape
   //
   //       0        7
   //       +--------+
   //      /          \
   //   1 +            + 6
   //     |            |
   //   2 +            + 5
   //      \          /
   //       +--------+
   //       3        4

   std::array<b2Vec2, 8> vertices {
      b2Vec2{bevel_m,             0.0f              },
      b2Vec2{0.0f,                bevel_m           },
      b2Vec2{0.0f,                height_m - bevel_m},
      b2Vec2{bevel_m,             height_m          },
      b2Vec2{width_m - bevel_m,   height_m          },
      b2Vec2{width_m,             height_m - bevel_m},
      b2Vec2{width_m,             bevel_m           },
      b2Vec2{width_m - bevel_m,   0.0f              },
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
   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "bubble_cube.png");
   _sprite.setTexture(*_texture);
   _sprite.setPosition(x + sprite_offset_x_px, y + sprite_offset_y_px);
}


void BubbleCube::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   auto sprite_index = 0;

   if (_popped)
   {
      sprite_index = std::min(static_cast<int32_t>(_pop_elapsed_s * pop_frequency), columns - 1);
   }
   else
   {
      sprite_index = static_cast<int32_t>(_mapped_value_normalized * columns + 6) % columns;
   }

   _sprite.setTextureRect({
         sprite_index * PIXELS_PER_TILE * tiles_per_box_width,
         (_popped ? 1 : 0) * PIXELS_PER_TILE * tiles_per_box_height,
         PIXELS_PER_TILE * tiles_per_box_width,
         PIXELS_PER_TILE * tiles_per_box_height
      }
   );

   color.draw(_sprite);
}


// 12 x 4 boxes per row
//
// regular animation is in row 0
// pop animation is in row 1
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+ . . .
// |   | __|__ |   |   | __|__ |   |   | __|__ |   |   | __|__ |   |
// +---+/#####\+---+---+/#####\+---+---+/#####\+---+---+/#####\+---+ . . .
// |   |#######|   |   |#######|   |   |#######|   |   |#######|   |
// +---+\#####/+---+---+\#####/+---+---+\#####/+---+---+\#####/+---+ . . .
// |   | ""|"" |   |   | ""|"" |   |   | ""|"" |   |   | ""|"" |   |
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+ . . .
//

void BubbleCube::update(const sf::Time& dt)
{
   _elapsed_s += dt.asSeconds();
   _pop_elapsed_s += dt.asSeconds();

   const auto mapped_value = fmod((_animation_offset_s + _elapsed_s) * move_frequency, static_cast<float>(M_PI) * 2.0f);
   _mapped_value_normalized = mapped_value / (static_cast<float>(M_PI) * 2.0f);

   const auto move_offset = move_amplitude * sin(mapped_value) * b2Vec2{0.0f, 1.0f};

   _body->SetTransform(_position_m + move_offset, 0.0f);

   if (_popped)
   {
      _body->SetActive(false);
   }

   // respawn when the time has come
   if (_popped && (GlobalClock::getInstance().getElapsedTime() - _pop_time).asSeconds() > _pop_time_respawn_s)
   {
      _popped = false;
      _body->SetActive(true);
   }
}


void BubbleCube::beginContact(FixtureNode* other)
{
   if (_pop_only_on_foot_contact && other->getType() != ObjectTypePlayerFootSensor)
   {
      return;
   }

   if (_popped)
   {
      return;
   }

   _contact_count++;
}


void BubbleCube::endContact(FixtureNode* other)
{
   if (_pop_only_on_foot_contact && other->getType() != ObjectTypePlayerFootSensor)
   {
      return;
   }

   if (_popped)
   {
      return;
   }

   _contact_count--;

   if (_contact_count == 0)
   {
      _popped = true;
      _pop_time = GlobalClock::getInstance().getElapsedTime();
      _pop_elapsed_s = 0.0f;
   }
}


