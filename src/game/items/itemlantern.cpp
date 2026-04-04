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

namespace
{

sf::Vector2f update_torch_offset(sf::Vector2f current_offset, sf::Vector2f target_offset, const sf::Time dt)
{
   const sf::Vector2f delta = target_offset - current_offset;
   const auto distance_sq = (delta.x * delta.x) + (delta.y * delta.y);
   constexpr auto base_speed = 0.5f;
   constexpr auto reference_distance_px = 5.0f;
   const auto reference_distance_sq = reference_distance_px * reference_distance_px;
   const auto normalized_sq = distance_sq / reference_distance_sq;
   const auto factor = std::clamp(base_speed * dt.asSeconds() * normalized_sq * normalized_sq, 0.0f, 1.0f);
   return current_offset + delta * factor;
}

}  // namespace

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

   // update light position to follow player with eye position offset
   auto* player = Player::getCurrent();
   if (!player || !_player_light)
   {
      return;
   }

   const auto& player_animation = player->getPlayerAnimation();
   if (!player_animation)
   {
      return;
   }

   const auto& current_cycle = player_animation->getCurrentCycle();
   if (!current_cycle)
   {
      return;
   }

#ifdef DEBUG_DRAW
   _light_circle.setPosition(player->getPixelPositionFloat());
#endif

   // get eye position in pixels, or reuse last valid position
   const auto& eye_positions = player->getEyePositions();
   const auto eye_pos_opt = eye_positions.getEyePosition(current_cycle);
   
   if (eye_pos_opt.has_value())
   {
      _last_valid_eye_position = eye_pos_opt.value();
   }
   
   // convert to meters
   const float eye_offset_x_m = _last_valid_eye_position.x * MPP;
   const float eye_offset_y_m = _last_valid_eye_position.y * MPP;
   
   // add static offset
   static constexpr float offset_x_m = -2.85f;
   static constexpr float offset_y_m = -0.6f;
   
   _player_light->_pos_m = player->getBody()->GetPosition() + b2Vec2(eye_offset_x_m + offset_x_m, eye_offset_y_m + offset_y_m);
   _player_light->updateSpritePosition();
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
