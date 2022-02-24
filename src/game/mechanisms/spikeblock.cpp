#include "spikeblock.h"

#include "framework/tmxparser/tmxobject.h"
#include "texturepool.h"
#include "player/player.h"


namespace
{
constexpr auto count_columns = 8;
constexpr auto count_rows = 5;
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
}


void SpikeBlock::updateSpriteRect()
{
   _sprite.setTextureRect({
      _tu_px * PIXELS_PER_TILE,
      _tv_px * PIXELS_PER_TILE,
      PIXELS_PER_TILE,
      PIXELS_PER_TILE}
   );
}


void SpikeBlock::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void SpikeBlock::update(const sf::Time& /*dt*/)
{
   if (Player::getCurrent()->getPlayerPixelRect().intersects(_rectangle))
   {
      Player::getCurrent()->damage(100);
   }

   updateSpriteRect();
}
