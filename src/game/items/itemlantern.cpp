// #define DEBUG_DRAW

#include "itemlantern.h"

#include <cmath>

#include "game/io/gamedeserializedata.h"
#include "game/io/texturepool.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

ItemLantern::ItemLantern()
{
   _light_circle.setRadius(_light_radius);
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

   _elapsed += dt;

   // update light position to follow player with downward offset
   auto* player = Player::getCurrent();
   if (player && _player_light)
   {
#ifdef DEBUG_DRAW
      _light_circle.setPosition(player->getPixelPositionFloat());
#endif
      const float offset_x_m = -2.85f;  // small horizontal sway
      const float offset_y_m = -0.6f + sin(_elapsed.asSeconds() * 3.0f) * 0.015f;
      _player_light->_pos_m = player->getBody()->GetPosition() + b2Vec2(offset_x_m, offset_y_m);
      _player_light->updateSpritePosition();
      // _player_light->_sprite->setOrigin({128, 64});
      // _player_light->_sprite->setRotation(sf::degrees(sin(_elapsed.asSeconds() * 3.0f) * 3.f));
      // _player_light->_sprite->setRotation(sf::degrees(sin(_elapsed.asSeconds() * 3.0f) * 0.2f));
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

   // load texture first to get actual dimensions
   const std::string texture_name = "spotlight.png";
   const std::string texture_path = "data/light/" + texture_name;
   auto texture = TexturePool::getInstance().get(texture_path);
   const auto texture_size = texture->getSize();

   // create mock GameDeserializeData
   GameDeserializeData data;
   data._tmx_object = std::make_shared<TmxObject>();
   data._tmx_object->_properties = std::make_shared<TmxProperties>();

   // set desired light quad display dimensions (smaller than before)
   constexpr float desired_width_px = 256.0f;
   constexpr float desired_height_px = 128.0f;
   data._tmx_object->_width_px = desired_width_px;
   data._tmx_object->_height_px = desired_height_px;

   // set texture property
   auto texture_property = std::make_shared<TmxProperty>();
   texture_property->_value_string = texture_name;
   data._tmx_object->_properties->_map["texture"] = texture_property;

   Log::Info() << "ItemLantern: Creating light with texture: " << texture_name 
               << " (texture size: " << texture_size.x << "x" << texture_size.y 
               << ", display size: " << desired_width_px << "x" << desired_height_px << ")";

   // calculate center offset based on light source position in texture
   // the light source is at the right edge, vertically centered
   auto center_offset_x_property = std::make_shared<TmxProperty>();
   auto center_offset_y_property = std::make_shared<TmxProperty>();
   center_offset_x_property->_value_int = static_cast<int32_t>(desired_width_px / 2);
   center_offset_y_property->_value_int = 0;  // static_cast<int32_t>(desired_height_px / 2);
   data._tmx_object->_properties->_map["center_offset_x_px"] = center_offset_x_property;
   data._tmx_object->_properties->_map["center_offset_y_px"] = center_offset_y_property;

   _player_light = LightSystem::createLightInstance(player, data);
   _player_light->_color = sf::Color(255, 200, 100, 255);
   _player_light->_sprite->setColor(_player_light->_color);

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
