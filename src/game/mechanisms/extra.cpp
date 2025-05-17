#include "extra.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/io/gamedeserializedata.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"
#include "game/player/playercontrols.h"
#include "game/player/playerinfo.h"
#include "game/state/savestate.h"

// #define DRAW_DEBUG 1
#ifdef DRAW_DEBUG
#include "game/debugdraw.h"
#endif

namespace
{
const auto registered_extra = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("Extra", "extras");

   registry.registerLayerName(
      "extras",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Extra>(parent);
         mechanism->deserialize(data);
         mechanisms["extras"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "Extra",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Extra>(parent);
         mechanism->deserialize(data);
         mechanisms["extras"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

Extra::Extra(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Extra).name());
}

void Extra::deserialize(const GameDeserializeData& data)
{
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   _name = data._tmx_object->_name;
   _rect = {{pos_x_px, pos_y_px}, {width_px, height_px}};

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;

      setZ(ValueReader::readValue<int32_t>("z", map).value_or(0));
      _active = ValueReader::readValue<bool>("active", map).value_or(true);
      _spawn_required = ValueReader::readValue<bool>("spawn_required", map).value_or(false);

      // if a texture is defined, no animation is used
      const auto texture_path = ValueReader::readValue<std::string>("texture", map).value_or("");
      if (!texture_path.empty())
      {
         _texture = TexturePool::getInstance().get(texture_path);
         _sprite = std::make_unique<sf::Sprite>(*_texture);
         _sprite->setPosition({pos_x_px, pos_y_px});

         // read texture rect
         sf::IntRect rect;
         rect.position.x = ValueReader::readValue<int32_t>("texture_rect_x", map).value_or(0);
         rect.position.y = ValueReader::readValue<int32_t>("texture_rect_y", map).value_or(0);
         rect.size.x = ValueReader::readValue<int32_t>("texture_rect_width", map).value_or(0);
         rect.size.y = ValueReader::readValue<int32_t>("texture_rect_height", map).value_or(0);

         if (rect.size.x > 0 && rect.size.y > 0)
         {
            _sprite->setTextureRect(rect);
         }
      }

      const auto sample_it = data._tmx_object->_properties->_map.find("sample");
      if (sample_it != data._tmx_object->_properties->_map.end())
      {
         _sample = sample_it->second->_value_string.value();
         Audio::getInstance().addSample(_sample.value());
      }

      const auto requires_action_button_it = data._tmx_object->_properties->_map.find("requires_button_press");
      if (requires_action_button_it != data._tmx_object->_properties->_map.end())
      {
         _requires_button_press = requires_action_button_it->second->_value_bool.value();
      }

      // read animations if set up
      const auto offset_x = width_px * 0.5f;
      const auto offset_y = height_px * 0.5f;
      AnimationPool animation_pool{"data/sprites/extra_animations.json"};
      const auto animation_pickup = data._tmx_object->_properties->_map.find("animation_pickup");
      if (animation_pickup != data._tmx_object->_properties->_map.end())
      {
         const auto key = animation_pickup->second->_value_string.value();
         _animation_pickup = animation_pool.create(key, pos_x_px + offset_x, pos_y_px + offset_y, false, false);
      }

      constexpr auto item_count_max = 99;
      for (auto i = 0; i < item_count_max; i++)
      {
         const auto main_animation_key = "animation_main_" + std::to_string(i);
         const auto main_animation_n = data._tmx_object->_properties->_map.find(main_animation_key);
         if (main_animation_n != data._tmx_object->_properties->_map.end())
         {
            const auto key = main_animation_n->second->_value_string.value();
            auto main_animation = animation_pool.create(key, pos_x_px + offset_x, pos_y_px + offset_y, false, false);
            _animations_main.push_back(main_animation);
         }
      }
   }

   if (!_animations_main.empty())
   {
      _animations_main_it = _animations_main.begin();
   }

   // add
   // - enable/disable mechanism function to level
   // - add enable/disable mechanism code to levelscript
}

void Extra::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (_spawn_required)
   {
      // draw spawn animation if we have one
      if (_animation_spawn && !_animation_spawn->_paused)
      {
         _animation_spawn->draw(target);
      }

      // don't draw item if not spawned yet
      if (!_spawned)
      {
         return;
      }
   }

   if (_animation_pickup && !_animation_pickup->_paused)
   {
      _animation_pickup->draw(target);
   }

   if (!_active)
   {
      return;
   }

   // draw animations
   if (!_animations_main.empty())
   {
      (*_animations_main_it)->draw(target);
   }

   // or show static extra texture
   else
   {
      target.draw(*_sprite);
   }

#ifdef DRAW_DEBUG
   DebugDraw::drawRect(target, _rect);
#endif
}

void Extra::update(const sf::Time& delta_time)
{
   if (_spawn_required)
   {
      if (_animation_spawn && !_animation_spawn->_paused)
      {
         _animation_spawn->update(delta_time);
      }

      // no pick up allowed if not spawned yet
      if (!_spawned)
      {
         return;
      }
   }

   // play pickup animation if active
   if (_animation_pickup && !_animation_pickup->_paused)
   {
      _animation_pickup->update(delta_time);
   }

   if (!_active)
   {
      return;
   }

   // update regular animations
   if (!_animations_main.empty())
   {
      (*_animations_main_it)->update(delta_time);
      if ((*_animations_main_it)->_paused)
      {
         _animations_main_it++;

         // start loop again if needed
         if (_animations_main_it == _animations_main.end())
         {
            _animations_main_it = _animations_main.begin();
         }

         (*_animations_main_it)->seekToStart();
         (*_animations_main_it)->play();
      }
   }

   if (_requires_button_press)
   {
      if (!Player::getCurrent()->getControls()->isButtonBPressed())
      {
         return;
      }
   }

   const auto& player_rect_px = Player::getCurrent()->getPixelRectFloat();
   if (player_rect_px.findIntersection(_rect).has_value())
   {
      _active = false;

      for (auto& callback : _callbacks)
      {
         callback(_name);
      }

      if (_sample.has_value())
      {
         Audio::getInstance().playSample({_sample.value()});
      }

      SaveState::getPlayerInfo()._inventory.add(_name);
   }
}

std::optional<sf::FloatRect> Extra::getBoundingBoxPx()
{
   return _rect;
}

void Extra::spawn()
{
   _spawned = true;

   if (_animation_spawn)
   {
      _animation_spawn->play();
   }
}
