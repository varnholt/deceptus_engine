// #define DEBUG_DRAW

#include "itemlantern.h"

#include <cmath>

#include "game/io/gamedeserializedata.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

ItemLantern::ItemLantern()
{
   _light_radius = 50.0f;
   _light_circle.setRadius(_light_radius);
   _light_circle.setFillColor(sf::Color(255, 200, 100, 150));
   _light_circle.setOrigin({_light_radius, _light_radius});
}

void ItemLantern::draw(sf::RenderTarget& target)
{
   if (!_enabled)
   {
      return;
   }

#ifdef DEBUG_DRAW
   target.draw(_light_circle);
#endif
}

void ItemLantern::update(const sf::Time& dt)
{
   if (!_enabled)
   {
      return;
   }

   // update light position to follow player with downward offset
   auto* player = Player::getCurrent();
   if (player && _player_light)
   {
#ifdef DEBUG_DRAW
      _light_circle.setPosition(player->getPixelPositionFloat());
#endif
      // offset the light downward so it appears to be "held" by the player
      constexpr float offset_x_m = -1.0f;  // offset downward by 0.5 meters
      constexpr float offset_y_m = -0.5f;  // offset downward by 0.5 meters
      _player_light->_pos_m = player->getBody()->GetPosition() + b2Vec2(offset_x_m, offset_y_m);
      _player_light->updateSpritePosition();
   }
}

void ItemLantern::onEquipped()
{
   _enabled = true;

   // create and add light instance to the light system
   auto* player = Player::getCurrent();
   if (!player)
   {
      return;
   }

   // create mock GameDeserializeData
   GameDeserializeData data;
   data._tmx_object = std::make_shared<TmxObject>();
   data._tmx_object->_properties = std::make_shared<TmxProperties>();

   // set light quad dimensions
   data._tmx_object->_width_px = 512.0f;
   data._tmx_object->_height_px = 512.0f;

   // set texture property to topdown.png
   auto texture_property = std::make_shared<TmxProperty>();
   texture_property->_value_string = "x-arrow.png";  // X-shaped pattern - VERY obvious if working
   data._tmx_object->_properties->_map["texture"] = texture_property;

   Log::Info() << "ItemLantern: Creating light with texture: " << texture_property->_value_string.value();

   // set center offset to move light down by 256px
   // auto center_offset_y_property = std::make_shared<TmxProperty>();
   // center_offset_y_property->_value_int = 256;
   // data._tmx_object->_properties->_map["center_offset_y_px"] = center_offset_y_property;

   _player_light = LightSystem::createLightInstance(player, data);
   _player_light->_color = sf::Color(255, 200, 100, 255);  // warm yellowish color
   // _player_light->_sprite->setRotation(sf::degrees(270));  // TEMPORARILY DISABLED

   Log::Info() << "ItemLantern: Light created, texture size: " << _player_light->_texture->getSize().x << "x" 
               << _player_light->_texture->getSize().y 
               << ", sprite scale: " << _player_light->_sprite->getScale().x
               << ", sprite color: " << (int)_player_light->_sprite->getColor().r << "," 
               << (int)_player_light->_sprite->getColor().g << ","
               << (int)_player_light->_sprite->getColor().b << ","
               << (int)_player_light->_sprite->getColor().a;

   auto* level = Level::getCurrentLevel();
   if (level && level->getLightSystem())
   {
      level->getLightSystem()->_lights.push_back(_player_light);
   }
}

void ItemLantern::onUnequipped()
{
   _enabled = false;

   // remove light instance from the light system
   if (_player_light)
   {
      auto* level = Level::getCurrentLevel();
      if (level && level->getLightSystem())
      {
         auto& lights = level->getLightSystem()->_lights;
         lights.erase(
            std::remove(lights.begin(), lights.end(), _player_light),
            lights.end()
         );
      }
      _player_light.reset();
   }
}

std::string ItemLantern::getName() const
{
   return "Lantern";
}
