#include "spikeblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "texturepool.h"
#include "player/player.h"


/*
   000
  +---+---+---+---+---+---+---+---+
  |   |   | . | . | . | o | o | * | enabled
  +---+---+---+---+---+---+---+---+
  | * | * | * | O | O |( )|( )|(#)|
  +---+---+---+---+---+---+---+---+
  |(A)|(#)|(#)|(#)|(#)|(#)|(#)|(#)| A = active sprite (16)
  +---+---+---+---+---+---+---+---+
  |(#)|(#)|(=)|(=)|(=)|(=)|(=)|(=)|
  +---+---+---+---+---+---+---+---+
  |( )| O | O | * | * | o | . |   | disabled
  +---+---+---+---+---+---+---+---+
                               039

*/

namespace
{
constexpr auto count_columns = 8;
constexpr auto count_rows = 5;
constexpr auto animation_speed = 1.0f;
}


SpikeBlock::SpikeBlock(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(SpikeBlock).name());
}


void SpikeBlock::deserialize(TmxObject* tmx_object)
{
   _texture_map = TexturePool::getInstance().get("data/sprites/enemy_spikeblock.png");
   _sprite.setTexture(*_texture_map);
   _sprite.setPosition(tmx_object->_x_px, tmx_object->_y_px);

   _rectangle = {
      static_cast<int32_t>(tmx_object->_x_px),
      static_cast<int32_t>(tmx_object->_y_px),
      static_cast<int32_t>(tmx_object->_width_px),
      static_cast<int32_t>(tmx_object->_height_px)
   };

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   updateSpriteRect();
}


void SpikeBlock::updateSpriteRect()
{
   _tu_tl = _sprite_index_current % 8;
   _tv_tl = _sprite_index_current / 8;

   _sprite.setTextureRect({
      _tu_tl * PIXELS_PER_TILE,
      _tv_tl * PIXELS_PER_TILE,
      PIXELS_PER_TILE,
      PIXELS_PER_TILE}
   );
}


const sf::IntRect& SpikeBlock::getPixelRect() const
{
   return _rectangle;
}


void SpikeBlock::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void SpikeBlock::update(const sf::Time& dt)
{
   if (Player::getCurrent()->getPlayerPixelRect().intersects(_rectangle))
   {
      Player::getCurrent()->damage(100);
   }

   if (_sprite_index_current != _sprite_index_target)
   {
      _sprite_value += dt.asSeconds() * animation_speed;

      const auto sprite_index = static_cast<int32_t>(std::floor(_sprite_value));

      if (sprite_index != _sprite_index_current)
      {
         _sprite_index_current = sprite_index;
         updateSpriteRect();
      }
   }
}


void SpikeBlock::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
   _sprite_index_target = (enabled ? 16 : 39);
}


