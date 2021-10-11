#include "bubblecube.h"

#include "framework/tmxparser/tmxobject.h"
#include "texturepool.h"


namespace
{
constexpr auto width_m  = 24 * MPP;
constexpr auto height_m = 24 * MPP;
constexpr auto bevel_m = 6 * MPP;
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
   b2FixtureDef boundary_fixture_def;
   boundary_fixture_def.shape = &_shape;
   boundary_fixture_def.density = 1.0f;
   boundary_fixture_def.isSensor = false;
   auto boundary_fixture = _body->CreateFixture(&boundary_fixture_def);
   boundary_fixture->SetUserData(static_cast<void*>(this));

   // set up visualization
   _texture = TexturePool::getInstance().get(base_path / "tilesets" / "bubble_cube.png");
   _sprite.setTexture(*_texture);
   _sprite.setPosition(x, y);
}


void BubbleCube::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   static constexpr auto animation_speed = 0.2f;
   static constexpr auto columns = 12;
   static constexpr auto tiles_per_box_width = 4;
   static constexpr auto tiles_per_box_height = 3;

   const auto val = static_cast<int32_t>(_elapsed * 40.0f * fabs(animation_speed)) % columns;

   const auto popped = false;

   _sprite.setTextureRect({
         val * PIXELS_PER_TILE,
         popped ? 1 : 0,
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
   _elapsed += dt.asSeconds();
}




