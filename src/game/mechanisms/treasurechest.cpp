#include "treasurechest.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/animationpool.h"
#include "game/audio.h"
#include "game/texturepool.h"
#include "game/valuereader.h"

TreasureChest::TreasureChest(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(TreasureChest).name());
}

void TreasureChest::deserialize(const GameDeserializeData& data)
{
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   _rect = {pos_x_px, pos_y_px, width_px, height_px};

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;
      _z_index = ValueReader::readValue<int32_t>("z", map).value_or(0);

      const auto texture_path = ValueReader::readValue<std::string>("texture", map).value_or("treasure_chest.png");
      _texture = TexturePool::getInstance().get(texture_path);
      _sprite.setTexture(*_texture);
      _sprite.setPosition(pos_x_px, pos_y_px);

      _sample_open = ValueReader::readValue<std::string>("sample", map).value_or("trasure_chest_open.wav");
      Audio::getInstance().addSample(_sample_open);

      // read animations if set up
      const auto offset_x = width_px * 0.5f;
      const auto offset_y = height_px * 0.5f;

      AnimationPool animation_pool{"data/sprites/treasure_animations.json"};

      _animation_idle_closed = animation_pool.create(
         ValueReader::readValue<std::string>("animation_idle_closed", map).value_or("idle_closed"),
         pos_x_px + offset_x,
         pos_y_px + offset_y,
         false,
         false
      );

      _animation_idle_opening = animation_pool.create(
         ValueReader::readValue<std::string>("animation_idle_opening", map).value_or("idle_opening"),
         pos_x_px + offset_x,
         pos_y_px + offset_y,
         false,
         false
      );

      _animation_idle_open = animation_pool.create(
         ValueReader::readValue<std::string>("animation_idle_open", map).value_or("idle_open"),
         pos_x_px + offset_x,
         pos_y_px + offset_y,
         false,
         false
      );
   }
}

void TreasureChest::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (_animation_idle_closed)
   {
      _animation_idle_closed->draw(target);
   }
}

void TreasureChest::update(const sf::Time& dt)
{
   if (_animation_idle_closed)
   {
      _animation_idle_closed->update(dt);
   }
}

std::optional<sf::FloatRect> TreasureChest::getBoundingBoxPx()
{
   return std::nullopt;
}
