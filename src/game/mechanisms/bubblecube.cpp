#include "bubblecube.h"

#include "framework/tmxparser/tmxobject.h"
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
static constexpr auto move_amplitude = 0.1f;
static constexpr auto move_frequency = 4.19f;

static constexpr auto pop_time_respawn_s = 3.0f;

static constexpr auto sprite_offset_x_px = -30;
static constexpr auto sprite_offset_y_px = -14;
}


BubbleCube::BubbleCube(
   GameNode* parent,
   const std::shared_ptr<b2World>& world,
   TmxObject* tmx_object,
   const std::filesystem::path& base_path
)
 : FixtureNode(parent)
{
   setName(typeid(BubbleCube).name());
   setType(ObjectTypeBubbleCube);

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
   const auto x = tmx_object->_x_px;
   const auto y = tmx_object->_y_px;
   _position_m = MPP * b2Vec2{x, y};

   b2BodyDef body_def;
   body_def.type = b2_staticBody;
   body_def.position = _position_m;
   _body = world->CreateBody(&body_def);

   // set up body fixture
   b2FixtureDef fixture_def;
   fixture_def.shape = &_shape;
   fixture_def.density = 1.0f;
   fixture_def.isSensor = false;
   _fixture = _body->CreateFixture(&fixture_def);
   _fixture->SetUserData(static_cast<void*>(this));

   // set up visualization
   _texture = TexturePool::getInstance().get(base_path / "tilesets" / "bubble_cube.png");
   _sprite.setTexture(*_texture);
   _sprite.setPosition(x + sprite_offset_x_px, y + sprite_offset_y_px);
}


void BubbleCube::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   auto val = static_cast<int32_t>(_elapsed_s * animation_speed);

   // popped animation is not looped
   if (_popped)
   {
      val = std::min(val, columns - 1);
   }
   else
   {
      val = val % columns;
   }

   _sprite.setTextureRect({
         val * PIXELS_PER_TILE * tiles_per_box_width,
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

   const auto move_offset = move_amplitude * -sin(_elapsed_s * move_frequency) * b2Vec2{0, 1};

   _body->SetTransform(_position_m + move_offset, 0.0f);

   // respawn when the time has come
   if (_popped && (GlobalClock::getInstance().getElapsedTime() - _pop_time).asSeconds() > pop_time_respawn_s)
   {
      _popped = false;
      _fixture->SetSensor(false);
   }
}


void BubbleCube::beginContact()
{
   _contact_count++;
}


void BubbleCube::endContact()
{
   _contact_count--;

   if (_contact_count == 0)
   {
      _popped = true;
      _pop_time = GlobalClock::getInstance().getElapsedTime();
      _fixture->SetSensor(true);
      _elapsed_s = 0.0f;
   }
}


