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

constexpr float offset_left_x_m = -3.4f;
constexpr float offset_right_x_m = 1.9f;
constexpr float offset_y_m = -1.0f;

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
    : _player_texture(TexturePool::getInstance().get("data/sprites/player.png")),
      _helmet_sprite_r(std::make_unique<sf::Sprite>(*_player_texture)),
      _helmet_sprite_l(std::make_unique<sf::Sprite>(*_player_texture))
{
   _light_circle.setRadius(_light_radius);
   _light_circle.setOrigin({_light_radius, _light_radius});

   _helmet_sprite_r->setTextureRect(sf::IntRect({0, 1776}, {24, 24}));
   _helmet_sprite_l->setTextureRect(sf::IntRect({24, 1776}, {24, 24}));
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

   auto* player = Player::getCurrent();
   if (!player)
   {
      return;
   }

   target.draw(player->isPointingRight() ? *_helmet_sprite_r : *_helmet_sprite_l);
}

void ItemLantern::update(const sf::Time& delta_time)
{
   if (!_enabled)
   {
      return;
   }

   _elapsed += delta_time;

   // update light position to follow player with eye position offset
   auto* player = Player::getCurrent();
   if (!player)
   {
      return;
   }

   const auto& player_animation = player->getPlayerAnimation();
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

   // switch active light based on player direction
   const bool pointing_right = player->isPointingRight();
   const float offset_x_m = pointing_right ? offset_right_x_m : offset_left_x_m;
   
   auto& active_light = pointing_right ? _player_light_right : _player_light_left;
   auto& inactive_light = pointing_right ? _player_light_left : _player_light_right;

   active_light->_enabled = true;
   inactive_light->_enabled = false;
   active_light->_pos_m = player->getBody()->GetPosition() + b2Vec2(eye_offset_x_m + offset_x_m, eye_offset_y_m + offset_y_m);
   active_light->updateSpritePosition();

   sf::Vector2f helmet_offset_px{pointing_right ? -50.0f : -45.0f, -54.0f};
   const auto helmet_position_px = player->getPixelPositionFloat() + _last_valid_eye_position + helmet_offset_px;
   _helmet_sprite_r->setPosition(helmet_position_px);
   _helmet_sprite_l->setPosition(helmet_position_px);
}

void ItemLantern::onEquipped()
{
   auto* player = Player::getCurrent();
   if (!player)
   {
      return;
   }

   auto* level = Level::getCurrentLevel();
   if (!level || !level->getLightSystem())
   {
      return;
   }

   constexpr float desired_width_px = 256.0f;
   constexpr float desired_height_px = 128.0f;

   Log::Info() << "ItemLantern: Creating directional lights";

   enum class Orientation : uint8_t
   {
      Left,
      Right
   };

   // helper to create an identical light instance
   auto create_light = [&](Orientation orientation)
   {
      const auto direction = (orientation == Orientation::Left) ? 1 : -1;
      const std::string texture_name = (orientation == Orientation::Left) ? "spotlight_l.png" : "spotlight_r.png";
      GameDeserializeData data;
      data._tmx_object = std::make_shared<TmxObject>();
      data._tmx_object->_properties = std::make_shared<TmxProperties>();
      data._tmx_object->_width_px = desired_width_px;
      data._tmx_object->_height_px = desired_height_px;

      auto texture_property = std::make_shared<TmxProperty>();
      texture_property->_value_string = texture_name;
      data._tmx_object->_properties->_map["texture"] = texture_property;

      auto center_offset_x_property = std::make_shared<TmxProperty>();
      auto center_offset_y_property = std::make_shared<TmxProperty>();
      center_offset_x_property->_value_int = direction * static_cast<int32_t>(desired_width_px / 2);
      center_offset_y_property->_value_int = 0;
      data._tmx_object->_properties->_map["center_offset_x_px"] = center_offset_x_property;
      data._tmx_object->_properties->_map["center_offset_y_px"] = center_offset_y_property;

      auto light = LightSystem::createLightInstance(player, data);
      light->_color = sf::Color(255, 200, 100, 255);
      light->_sprite->setColor(light->_color);
      light->_enabled = false;

      return light;
   };

   _player_light_left = create_light(Orientation::Left);
   level->getLightSystem()->_lights.push_back(_player_light_left);

   _player_light_right = create_light(Orientation::Right);
   level->getLightSystem()->_lights.push_back(_player_light_right);

   _enabled = true;
}

void ItemLantern::onUnequipped()
{
   _enabled = false;

   auto* level = Level::getCurrentLevel();
   if (level && level->getLightSystem())
   {
      auto& lights = level->getLightSystem()->_lights;
      
      if (_player_light_left)
      {
         lights.erase(std::remove(lights.begin(), lights.end(), _player_light_left), lights.end());
         _player_light_left.reset();
      }
      
      if (_player_light_right)
      {
         lights.erase(std::remove(lights.begin(), lights.end(), _player_light_right), lights.end());
         _player_light_right.reset();
      }
   }
}

std::string ItemLantern::getName() const
{
   return "Lantern";
}
