#include "onoffblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "texturepool.h"
#include "player/player.h"

/*

   +---+---+---+---+---+---+---+---+
   |000|001|002|003|004|005|006|007| fade in
   +---+---+---+---+---+---+---+---+
   |008|...|   |   |   |   |   |014| fully visible
   +---+---+---+---+---+---+---+---+
   |016|...|   |   |   |   |   |   | fade out
   +---+---+---+---+---+---+---+---+
   |024|025|026|027|028|029|030|031| invisible again
   +---+---+---+---+---+---+---+---+
*/

namespace
{
static constexpr auto width_px = 36;
static constexpr auto height_px = 36;
static constexpr auto bevel_px = 6;
static constexpr auto width_m  = width_px * MPP;
static constexpr auto height_m = height_px * MPP;
static constexpr auto bevel_m = bevel_px * MPP;

static constexpr auto count_columns = 8;
static constexpr auto animation_speed = 40.0f;
}


OnOffBlock::OnOffBlock(GameNode* parent)
   : GameNode(parent)
{
   setClassName(typeid(OnOffBlock).name());
}


void OnOffBlock::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _texture_map = TexturePool::getInstance().get("data/sprites/on_off_block.png");
   _sprite.setTexture(*_texture_map);
   _sprite.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px);

   _rectangle = {
      static_cast<int32_t>(data._tmx_object->_x_px),
      static_cast<int32_t>(data._tmx_object->_y_px),
      static_cast<int32_t>(data._tmx_object->_width_px),
      static_cast<int32_t>(data._tmx_object->_height_px)
   };

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         const auto enabled = static_cast<bool>(enabled_it->second->_value_bool.value());
         setEnabled(enabled);
      }

      const auto mode_it = data._tmx_object->_properties->_map.find("mode");
      if (mode_it != data._tmx_object->_properties->_map.end())
      {
         auto mode_str = static_cast<std::string>(mode_it->second->_value_string.value());
         if (mode_str == "interval")
         {
            _mode = Mode::Interval;
         }
      }

      const auto time_on_it = data._tmx_object->_properties->_map.find("time_on_ms");
      if (time_on_it != data._tmx_object->_properties->_map.end())
      {
         _time_on_ms = static_cast<int32_t>(time_on_it->second->_value_int.value());
      }

      const auto time_off_it = data._tmx_object->_properties->_map.find("time_off_ms");
      if (time_on_it != data._tmx_object->_properties->_map.end())
      {
         _time_off_ms = static_cast<int32_t>(time_off_it->second->_value_int.value());
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
   _position_m = MPP * b2Vec2{data._tmx_object->_x_px, data._tmx_object->_y_px};

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

   updateSpriteRect();
}


void OnOffBlock::updateSpriteRect()
{
   _tu_tl = _sprite_index_current % count_columns;
   _tv_tl = _sprite_index_current / count_columns;

   _sprite.setTextureRect({
       _tu_tl * PIXELS_PER_TILE,
       _tv_tl * PIXELS_PER_TILE,
       PIXELS_PER_TILE,
       PIXELS_PER_TILE}
   );
}


const sf::IntRect& OnOffBlock::getPixelRect() const
{
   return _rectangle;
}


void OnOffBlock::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void OnOffBlock::update(const sf::Time& dt)
{
   if (_mode == Mode::Interval)
   {
      _elapsed += dt;

      if (!isEnabled() && _elapsed.asMilliseconds() >= _time_off_ms)
      {
         setEnabled(true);
         _elapsed = {};
      }
      else if (isEnabled() && _elapsed.asMilliseconds() >= _time_on_ms)
      {
         setEnabled(false);
         _elapsed = {};
      }
   }

   if (_sprite_index_current != _sprite_index_target)
   {
      _sprite_value += dt.asSeconds() * animation_speed;

      const auto sprite_index = static_cast<int32_t>(std::floor(_sprite_value));

      if (sprite_index != _sprite_index_current)
      {
         // reset after completing animation cycle
         if (sprite_index >= _sprite_index_disabled)
         {
            _sprite_value = 0.0f;
            _sprite_index_current = 0;
            _sprite_index_target = 0;
         }
         else
         {
            _sprite_index_current = sprite_index;
         }

         updateSpriteRect();
      }
   }
}


void OnOffBlock::setEnabled(bool enabled)
{
   _body->SetActive(enabled);
   GameMechanism::setEnabled(enabled);
   _sprite_index_target = (enabled ? _sprite_index_enabled : _sprite_index_disabled);
}


