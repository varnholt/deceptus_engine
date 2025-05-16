#include "spikeblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

/*
   000
  +---+---+---+---+---+---+---+---+
  |   |   | . | . | . | o | o | * | enabled
  +---+---+---+---+---+---+---+---+
  | * | * | * | O | O |( )|( )|(#)|
  +---+---+---+---+---+---+---+---+
  |(#)|(#)|(#)|(#)|(#)|(#)|(#)|(#)|
  +---+---+---+---+---+---+---+---+
  |(#)|(#)|(=)|(=)|(=)|(=)|(=)|(=)| active
  +---+---+---+---+---+---+---+---+
  |( )| O | O | * | * | o | . |   | disabled
  +---+---+---+---+---+---+---+---+
                               039

*/

namespace
{
const auto registered_spikeblock = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("SpikeBlock", "spike_blocks");

   registry.registerLayerName(
      "spike_blocks",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SpikeBlock>(parent);
         mechanism->setup(data);
         mechanisms["spike_blocks"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "SpikeBlock",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<SpikeBlock>(parent);
         mechanism->setup(data);
         mechanisms["spike_blocks"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
constexpr auto count_columns = 8;
constexpr auto animation_speed = 40.0f;
constexpr auto damage = 100;
}  // namespace

SpikeBlock::SpikeBlock(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(SpikeBlock).name());
}

void SpikeBlock::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _texture_map = TexturePool::getInstance().get("data/sprites/enemy_spikeblock.png");
   _sprite = std::make_unique<sf::Sprite>(*_texture_map);
   _sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});

   _rectangle = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   addChunks(_rectangle);

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

   updateSpriteRect();
}

void SpikeBlock::updateSpriteRect()
{
   _tu_tl = _sprite_index_current % count_columns;
   _tv_tl = _sprite_index_current / count_columns;

   _sprite->setTextureRect({{_tu_tl * PIXELS_PER_TILE, _tv_tl * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
}

const sf::FloatRect& SpikeBlock::getPixelRect() const
{
   return _rectangle;
}

void SpikeBlock::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(*_sprite);
}

void SpikeBlock::update(const sf::Time& dt)
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

   if (Player::getCurrent()->getPixelRectFloat().findIntersection(_rectangle).has_value())
   {
      if (_sprite_index_current >= _sprite_index_deadly_min && _sprite_index_current <= _sprite_index_deadly_max)
      {
         Player::getCurrent()->damage(damage);
      }
   }

   if (_sprite_index_current != _sprite_index_target)
   {
      _sprite_value += dt.asSeconds() * animation_speed;

      const auto sprite_index = static_cast<int32_t>(std::floor(_sprite_value));

      if (sprite_index != _sprite_index_current)
      {
         // reset after completing a full animation cycle
         if (sprite_index >= _sprite_index_disabled)
         {
            _sprite_value = 0.0f;
            _sprite_index_current = 0;
            _sprite_index_target = 0;
         }
         else
         {
            // otherwise just increase the index to either the enabled or disabled index
            _sprite_index_current = sprite_index;
         }

         updateSpriteRect();
      }
   }

   // pop from the queue to set up the next target state
   else if (!_target_states.empty())
   {
      const auto enabled = _target_states.front();
      GameMechanism::setEnabled(enabled);
      _sprite_index_target = enabled ? _sprite_index_enabled : _sprite_index_disabled;
      _target_states.pop_front();
   }
}

void SpikeBlock::setEnabled(bool enabled)
{
   // a queue is used here because the player might be hammering the lever
   _target_states.push_back(enabled);
}

std::optional<sf::FloatRect> SpikeBlock::getBoundingBoxPx()
{
   return _rectangle;
}
