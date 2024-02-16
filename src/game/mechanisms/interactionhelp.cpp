#include "interactionhelp.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "player/player.h"
#include "texturepool.h"

InteractionHelp::InteractionHelp(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(InteractionHelp).name());
}

void InteractionHelp::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   _animation_show->draw(target);
   _animation_hide->draw(target);
}

// workflow
// 1) on intersection
//    if hide is not playing
//    play show
// 2) show last show frame (show is paused)
// 3) if show is paused
//    if no intersection
//    play hide

void InteractionHelp::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto intersects = (player_rect.intersects(_rect_px));

   if (intersects && !_player_intersected_in_last_frame && _animation_hide->_paused)
   {
      // show
      _animation_hide->setVisible(false);
      _animation_show->setVisible(true);
      _animation_show->seekToStart();
      _animation_show->play();
      _active = true;
   }
   else if (!intersects && _animation_show->_paused && _active)
   {
      // hide
      _animation_show->setVisible(false);
      _animation_hide->setVisible(true);
      _animation_hide->seekToStart();
      _animation_hide->play();
      _active = false;
   }

   if (!_animation_show->_paused)
   {
      _animation_show->update(dt);
   }

   if (!_animation_hide->_paused)
   {
      _animation_hide->update(dt);
   }

   _player_intersected_in_last_frame = intersects;
}

void InteractionHelp::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   setZ(static_cast<int32_t>(ZDepth::ForegroundMax));

   _rect_px = sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   const auto z_it = data._tmx_object->_properties->_map.find("z");
   if (z_it != data._tmx_object->_properties->_map.end())
   {
      const auto z_index = static_cast<int32_t>(z_it->second->_value_int.value());
      setZ(z_index);
   }

   // read animations if set up
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;

   AnimationPool animation_pool{"data/sprites/interaction_animations.json"};

   const auto animation_name_show = data._tmx_object->_properties->_map.find("animation");
   if (animation_name_show != data._tmx_object->_properties->_map.end())
   {
      const auto key = animation_name_show->second->_value_string.value();

      _animation_show = animation_pool.create(key, pos_x_px, pos_y_px, false, false);
      _animation_show->_looped = false;
      _animation_show->_reset_to_first_frame = false;

      _animation_hide = animation_pool.create(key, pos_x_px, pos_y_px, false, false);
      _animation_hide->_looped = false;
      _animation_hide->_reset_to_first_frame = false;
      _animation_hide->reverse();
   }
}

std::optional<sf::FloatRect> InteractionHelp::getBoundingBoxPx()
{
   return _rect_px;
}
