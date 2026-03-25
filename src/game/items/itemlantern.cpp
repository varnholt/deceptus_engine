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
      constexpr float offset_y_m = 0.3f;  // offset downward by 0.5 meters
      _player_light->_pos_m = player->getBody()->GetPosition() + b2Vec2(0.0f, offset_y_m);
      _player_light->updateSpritePosition();
   }
}

void ItemLantern::onEquipped()
{
   _enabled = true;

   // create and add light instance to the light system
   auto* player = Player::getCurrent();
   if (player)
   {
      // create mock GameDeserializeData with texture property
      GameDeserializeData data;
      data._tmx_object = std::make_shared<TmxObject>();
      data._tmx_object->_properties = std::make_shared<TmxProperties>();
      
      // set light dimensions (in pixels)
      data._tmx_object->_width_px = 256.0f;
      data._tmx_object->_height_px = 256.0f;
      
      // set texture property to topdown.png
      auto texture_property = std::make_shared<TmxProperty>();
      texture_property->_value_string = "topdown.png";
      data._tmx_object->_properties->_map["texture"] = texture_property;

      _player_light = LightSystem::createLightInstance(player, data);
      _player_light->_color = sf::Color(255, 200, 100, 70);  // warm yellowish color

      auto* level = Level::getCurrentLevel();
      if (level && level->getLightSystem())
      {
         level->getLightSystem()->_lights.push_back(_player_light);
      }
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
