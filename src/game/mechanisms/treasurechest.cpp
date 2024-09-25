#include "treasurechest.h"

#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/animation/animationpool.h"
#include "game/audio/audio.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/extrawrapper.h"
#include "game/player/player.h"

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

   if (!data._tmx_object->_properties)
   {
      return;
   }

   const auto& map = data._tmx_object->_properties->_map;
   _z_index = ValueReader::readValue<int32_t>("z", map).value_or(0);

   const auto texture_path = ValueReader::readValue<std::string>("texture", map).value_or("data/sprites/treasure_chest.png");
   _texture = TexturePool::getInstance().get(texture_path);
   _sprite.setTexture(*_texture);
   _sprite.setPosition(pos_x_px, pos_y_px);

   _sample_open = ValueReader::readValue<std::string>("sample", map).value_or("treasure_chest_open.wav");
   Audio::getInstance().addSample(_sample_open);

   const auto spawn_extra = ValueReader::readValue<std::string>("spawn_extra", map).value_or("");
   if (!spawn_extra.empty())
   {
      _spawn_extra = spawn_extra;
   }

   // read animations if set up
   const auto offset_x = width_px * 0.5f;
   const auto offset_y = height_px * 0.5f;

   AnimationPool animation_pool{"data/sprites/treasure_chest_animations.json"};

   _animation_idle_closed = animation_pool.create(
      ValueReader::readValue<std::string>("animation_idle_closed", map).value_or("idle"),
      pos_x_px + offset_x,
      pos_y_px + offset_y,
      false,
      false
   );

   _animation_opening = animation_pool.create(
      ValueReader::readValue<std::string>("animation_opening", map).value_or("opening"),
      pos_x_px + offset_x,
      pos_y_px + offset_y,
      false,
      false
   );

   _animation_idle_open = animation_pool.create(
      ValueReader::readValue<std::string>("animation_idle_open", map).value_or("open"),
      pos_x_px + offset_x,
      pos_y_px + offset_y,
      false,
      false
   );

   _animation_idle_closed->_looped = true;
   _animation_idle_open->_looped = true;
   _animation_opening->_reset_to_first_frame = false;

   _spawn_effect = std::make_unique<SpawnEffect>(sf::Vector2f{_rect.left + _rect.width / 2, _rect.top - _rect.height / 2});
   _spawn_effect->deserialize(data);
}

void TreasureChest::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (_animation_idle_closed && _state == State::Closed)
   {
      _animation_idle_closed->draw(target);
   }
   else if (_animation_opening && _state == State::Opening)
   {
      _animation_opening->draw(target);
   }
   else if (_animation_idle_open && _state == State::Open)
   {
      _animation_idle_open->draw(target);
   }

   if (_spawn_effect->isActive())
   {
      _spawn_effect->draw(target);
   }
}

void TreasureChest::update(const sf::Time& dt)
{
   switch (_state)
   {
      case State::Closed:
      {
         if (_animation_idle_closed)
         {
            if (_animation_idle_closed->_paused)
            {
               _animation_idle_closed->seekToStart();
               _animation_idle_closed->play();
            }

            _animation_idle_closed->update(dt);
         }

         if (Player::getCurrent()->getControls()->isButtonBPressed())
         {
            const auto& player_rect_px = Player::getCurrent()->getPixelRectFloat();
            if (player_rect_px.intersects(_rect))
            {
               _spawn_effect->activate();

               _state = State::Opening;
               _animation_opening->seekToStart();
               _animation_opening->play();
               _animation_idle_closed->pause();
            }
         }

         break;
      }

      case State::Opening:
      {
         if (_animation_opening)
         {
            if (!_animation_opening->_paused)
            {
               _animation_opening->update(dt);
            }
            else
            {
               _state = State::Open;
               _animation_idle_open->seekToStart();
               _animation_idle_open->play();
               _animation_opening->pause();
            }
         }

         break;
      }

      case State::Open:
      {
         if (_animation_idle_open)
         {
            _animation_idle_open->update(dt);
         }
         break;
      }
   }

   if (_spawn_effect->isActive())
   {
      _spawn_effect->update(dt);

      if (_spawn_effect->isShown())
      {
         if (_spawn_extra.has_value())
         {
            ExtraWrapper::spawnExtra(_spawn_extra.value());
         }
      }
   }
}

std::optional<sf::FloatRect> TreasureChest::getBoundingBoxPx()
{
   return std::nullopt;
}
