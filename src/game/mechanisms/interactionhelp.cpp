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

void InteractionHelp::update(const sf::Time& /*dt*/)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   const auto intersects = (player_rect.intersects(_rect_px));
   bool state_changed = false;

   if (intersects && !_player_intersected_in_last_frame)
   {
      // show
      _animation_show->play();
      state_changed = true;
   }
   else if (!intersects && _player_intersected_in_last_frame)
   {
      // hide
      _animation_hide->play();
      state_changed = true;
   }

   if (state_changed)
   {
      _animation_hide->setVisible(!intersects);
      _animation_show->setVisible(intersects);
   }

   _player_intersected_in_last_frame = intersects;
}

void InteractionHelp::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   setZ(static_cast<int32_t>(ZDepth::Player) - 1);

   _rect_px = sf::FloatRect{data._tmx_object->_x_px, data._tmx_object->_y_px, data._tmx_object->_width_px, data._tmx_object->_height_px};

   // read animations if set up
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;

   AnimationPool animation_pool{"data/sprites/interaction_animations.json"};

   const auto animation_name_show = data._tmx_object->_properties->_map.find("animation_show");
   if (animation_name_show != data._tmx_object->_properties->_map.end())
   {
      const auto key = animation_name_show->second->_value_string.value();
      _animation_show = animation_pool.create(key, pos_x_px, pos_y_px, false, false);
   }

   const auto animation_name_hide = data._tmx_object->_properties->_map.find("animation_hide");
   if (animation_name_hide != data._tmx_object->_properties->_map.end())
   {
      const auto key = animation_name_hide->second->_value_string.value();
      _animation_hide = animation_pool.create(key, pos_x_px, pos_y_px, false, false);
   }
}

std::optional<sf::FloatRect> InteractionHelp::getBoundingBoxPx()
{
   return _rect_px;
}
