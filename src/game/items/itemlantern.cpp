// #define DEBUG_DRAW

#include "itemlantern.h"

#include <fstream>

#include "framework/easings/easings.h"
#include "game/io/texturepool.h"
#include "game/level/levelregistry.h"
#include "game/player/player.h"
#include "game/player/playerregistry.h"

ItemLantern::ItemLantern()
    : _player_texture(TexturePool::getInstance().get("data/sprites/player.png")),
#ifdef __EMSCRIPTEN__
      _helmet_sprite_r(std::make_unique<sf::Sprite>()),
      _helmet_sprite_l(std::make_unique<sf::Sprite>())
#else
      _helmet_sprite_r(std::make_unique<sf::Sprite>(*_player_texture)),
      _helmet_sprite_l(std::make_unique<sf::Sprite>(*_player_texture))
#endif
{
   _light_circle.setRadius(_light_radius);
#ifdef __EMSCRIPTEN__
   _light_circle.origin = {_light_radius, _light_radius};
#else
   _light_circle.setOrigin({_light_radius, _light_radius});
#endif

#ifdef __EMSCRIPTEN__
   _helmet_sprite_r->textureRect = sf::FloatRect{{0.0f, 1776.0f}, {24.0f, 24.0f}};
   _helmet_sprite_l->textureRect = sf::FloatRect{{24.0f, 1776.0f}, {24.0f, 24.0f}};
#else
   _helmet_sprite_r->setTextureRect(sf::IntRect({0, 1776}, {24, 24}));
   _helmet_sprite_l->setTextureRect(sf::IntRect({24, 1776}, {24, 24}));
#endif
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

   auto player = PlayerRegistry::getFirst();
   if (!player || player->isDead() || !_last_valid_eye_position.has_value())
   {
      return;
   }

#ifdef __EMSCRIPTEN__
   target.draw(player->isPointingRight() ? *_helmet_sprite_r : *_helmet_sprite_l, sf::RenderStates{.texture = _player_texture.get()});
#else
   target.draw(player->isPointingRight() ? *_helmet_sprite_r : *_helmet_sprite_l);
#endif
}

void ItemLantern::update(const sf::Time& delta_time)
{
   if (!_enabled)
   {
      return;
   }

   _elapsed += delta_time;

   // update light position to follow player with eye position offset
   auto player = PlayerRegistry::getFirst();
   if (!player || player->isDead())
   {
      _player_light_left->_enabled = false;
      _player_light_right->_enabled = false;
      _last_valid_eye_position.reset();
      _was_eye_position_valid = false;
      return;
   }

   const auto& player_animation = player->getPlayerAnimation();
   const auto& current_cycle = player_animation->getCurrentCycle();

#ifdef DEBUG_DRAW
#ifdef __EMSCRIPTEN__
   _light_circle.position = player->getPixelPositionFloat();
#else
   _light_circle.setPosition(player->getPixelPositionFloat());
#endif
#endif

   // only update the eye position while the animation is actually visible;
   // when invisible (hidden delay before appear animation), keep _last_valid_eye_position
   // as nullopt so the lights stay off via the has_value() check below
   if (current_cycle && current_cycle->isVisible())
   {
      const auto& eye_positions = player->getEyePositions();
      const auto eye_pos_opt = eye_positions.getEyePosition(current_cycle);
      if (eye_pos_opt.has_value())
      {
         _last_valid_eye_position = eye_pos_opt.value();
      }
   }

   // detect rising edge of valid eye position to trigger fade-in on spawn
   const bool eye_position_valid_now = _last_valid_eye_position.has_value();
   if (eye_position_valid_now && !_was_eye_position_valid)
   {
      _fade_in_elapsed = _fade_in_duration;
   }
   _was_eye_position_valid = eye_position_valid_now;

   // switch active light based on player direction
   const bool pointing_right = player->isPointingRight();
   const float offset_x_m = pointing_right ? _offset_right_x_m : _offset_left_x_m;

   auto& active_light = pointing_right ? _player_light_right : _player_light_left;
   auto& inactive_light = pointing_right ? _player_light_left : _player_light_right;

   if (!_last_valid_eye_position.has_value())
   {
      active_light->_enabled = false;
      inactive_light->_enabled = false;
      return;
   }

   // convert to meters
   const float eye_offset_x_m = _last_valid_eye_position->x * MPP;
   const float eye_offset_y_m = _last_valid_eye_position->y * MPP;

   active_light->_enabled = !PlayerRegistry::getFirst()->isDead();
   inactive_light->_enabled = false;

   // detect rising edges for hard landing (jitter + dust burst) and any landing (angle tilt)
   const bool currently_hard_landing = player->isHardLanding();
   if (currently_hard_landing && !_was_hard_landing)
   {
      _jitter_elapsed = _jitter_duration;
      _dust_burst_elapsed = _dust_burst_duration;
   }
   _was_hard_landing = currently_hard_landing;

   const bool currently_on_ground = player->isOnGround();
   if (currently_on_ground && !_was_on_ground)
   {
      _landing_tilt_elapsed = _landing_tilt_duration;
   }
   _was_on_ground = currently_on_ground;

   // advance all timers
   _jitter_elapsed = std::max(sf::Time{}, _jitter_elapsed - delta_time);
   _landing_tilt_elapsed = std::max(sf::Time{}, _landing_tilt_elapsed - delta_time);
   _dust_burst_elapsed = std::max(sf::Time{}, _dust_burst_elapsed - delta_time);
   _fade_in_elapsed = std::max(sf::Time{}, _fade_in_elapsed - delta_time);
   const float fade_alpha_factor =
      (_fade_in_elapsed > sf::Time{})
         ? static_cast<float>(Easings::easeOutSine(1.0f - _fade_in_elapsed.asSeconds() / _fade_in_duration.asSeconds()))
         : 1.0f;

   // base light position
   const float offset_y_m = pointing_right ? _offset_right_y_m : _offset_left_y_m;
   b2Vec2 light_pos_m = player->getBody()->GetPosition() + b2Vec2(eye_offset_x_m + offset_x_m, eye_offset_y_m + offset_y_m);

   // apply jitter: two sine waves at incommensurate frequencies decaying over the animation window
   if (_jitter_elapsed > sf::Time{})
   {
      const float jitter_t = _jitter_elapsed.asSeconds() / _jitter_duration.asSeconds();
      const float elapsed_s = _elapsed.asSeconds();
      light_pos_m +=
         b2Vec2(std::sin(elapsed_s * 47.3f) * _jitter_magnitude_m * jitter_t, std::sin(elapsed_s * 61.7f) * _jitter_magnitude_m * jitter_t);
   }

   // reset origin before updateSpritePosition so it always positions from (0, 0)
#ifdef __EMSCRIPTEN__
   active_light->_sprite->origin = {0.0f, 0.0f};
#else
   active_light->_sprite->setOrigin({0.0f, 0.0f});
#endif
   active_light->_pos_m = light_pos_m;
   active_light->updateSpritePosition();

   // set rotation origin at the lamp end in local texture coords (512x512 texture, scaled to display size)
#ifdef __EMSCRIPTEN__
   const sf::Vector2f sprite_scale = active_light->_sprite->scale;
#else
   const sf::Vector2f sprite_scale = active_light->_sprite->getScale();
#endif
   const float lamp_origin_y_local = static_cast<float>(active_light->_texture->getSize().y) * 0.5f;
   const float lamp_origin_x_local = pointing_right ? 0.0f : static_cast<float>(active_light->_texture->getSize().x);
#ifdef __EMSCRIPTEN__
   const sf::Vector2f top_left_pos = active_light->_sprite->position;
   active_light->_sprite->origin = {lamp_origin_x_local, lamp_origin_y_local};
   active_light->_sprite->position = {
      top_left_pos.x + lamp_origin_x_local * sprite_scale.x, top_left_pos.y + lamp_origin_y_local * sprite_scale.y
   };
#else
   const sf::Vector2f top_left_pos = active_light->_sprite->getPosition();
   active_light->_sprite->setOrigin({lamp_origin_x_local, lamp_origin_y_local});
   active_light->_sprite->setPosition(
      {top_left_pos.x + lamp_origin_x_local * sprite_scale.x, top_left_pos.y + lamp_origin_y_local * sprite_scale.y}
   );
#endif

   // easeOutSine brings the beam smoothly back to neutral after a landing tilt
   sf::Angle tilt_angle = sf::degrees(0.0f);
   if (_landing_tilt_elapsed > sf::Time{})
   {
      const float normalized_t = 1.0f - (_landing_tilt_elapsed.asSeconds() / _landing_tilt_duration.asSeconds());
      const float tilt_degrees = _landing_tilt_max_degrees * (1.0f - static_cast<float>(Easings::easeOutSine(normalized_t)));
      const float tilt_direction = pointing_right ? 1.0f : -1.0f;
      tilt_angle = sf::degrees(tilt_degrees * tilt_direction);
   }
#ifdef __EMSCRIPTEN__
   active_light->_sprite->rotation = tilt_angle;
#else
   active_light->_sprite->setRotation(tilt_angle);
#endif
   active_light->_color.a = static_cast<uint8_t>(static_cast<float>(_target_alpha) * fade_alpha_factor);

   // reset rotation on the inactive light so it is clean when it next becomes active
#ifdef __EMSCRIPTEN__
   inactive_light->_sprite->rotation = sf::degrees(0.0f);
#else
   inactive_light->_sprite->setRotation(sf::degrees(0.0f));
#endif

   sf::Vector2f helmet_offset_px{pointing_right ? -50.0f : -45.0f, -54.0f};
   const auto helmet_position_px = player->getPixelPositionFloat() + _last_valid_eye_position.value() + helmet_offset_px;
#ifdef __EMSCRIPTEN__
   _helmet_sprite_r->position = helmet_position_px;
   _helmet_sprite_l->position = helmet_position_px;
#else
   _helmet_sprite_r->setPosition(helmet_position_px);
   _helmet_sprite_l->setPosition(helmet_position_px);
#endif
   const uint8_t helmet_alpha = static_cast<uint8_t>(255.0f * fade_alpha_factor);
#ifdef __EMSCRIPTEN__
   _helmet_sprite_r->color = sf::Color(255, 255, 255, helmet_alpha);
   _helmet_sprite_l->color = sf::Color(255, 255, 255, helmet_alpha);
#else
   _helmet_sprite_r->setColor(sf::Color(255, 255, 255, helmet_alpha));
   _helmet_sprite_l->setColor(sf::Color(255, 255, 255, helmet_alpha));
#endif
}

void ItemLantern::onEquipped()
{
   auto player = std::static_pointer_cast<Player>(PlayerRegistry::getFirst());
   if (!player)
   {
      return;
   }

   auto level = LevelRegistry::getCurrent();
   if (!level || !level->getLightSystem())
   {
      return;
   }

   nlohmann::json config;
   std::ifstream("data/config/player_lantern.json") >> config;

   if (const auto it = config["left"].find("offset_x_px"); it != config["left"].end())
   {
      _offset_left_x_m = it->get<float>() * MPP;
   }

   if (const auto it = config["left"].find("offset_y_px"); it != config["left"].end())
   {
      _offset_left_y_m = it->get<float>() * MPP;
   }

   if (const auto it = config["right"].find("offset_x_px"); it != config["right"].end())
   {
      _offset_right_x_m = it->get<float>() * MPP;
   }

   if (const auto it = config["right"].find("offset_y_px"); it != config["right"].end())
   {
      _offset_right_y_m = it->get<float>() * MPP;
   }

   _player_light_left = LightSystem::createLightInstance(player.get(), config["left"]);
   _player_light_left->_enabled = false;
   level->getLightSystem()->_lights.push_back(_player_light_left);

   _player_light_right = LightSystem::createLightInstance(player.get(), config["right"]);
   _player_light_right->_enabled = false;
   level->getLightSystem()->_lights.push_back(_player_light_right);
   _target_alpha = _player_light_left->_color.a;

   const auto& dust_config = config["dust"];
   const bool dust_enabled = dust_config.value("enabled", false);
   const float dust_intensity = dust_config.value("intensity", 1.0f);
   const float layer_1_size = dust_config.value("layer_1_size", 6.0f);
   const float layer_1_speed_x = dust_config.value("layer_1_speed_x", 3.0f);
   const float layer_1_speed_y = dust_config.value("layer_1_speed_y", -2.0f);
   const float layer_2_size = dust_config.value("layer_2_size", 4.0f);
   const float layer_2_speed_x = dust_config.value("layer_2_speed_x", -2.0f);
   const float layer_2_speed_y = dust_config.value("layer_2_speed_y", 3.5f);

   _flicker_speed = config["flicker"].value("speed", 3.0f);
   _flicker_amount = config["flicker"].value("amount", 0.12f);
   _jitter_duration = sf::seconds(config["jitter"].value("duration_s", 0.4f));
   _jitter_magnitude_m = config["jitter"].value("magnitude_m", 0.12f);
   _landing_tilt_duration = sf::seconds(config["landing_tilt"].value("duration_s", 0.8f));
   _landing_tilt_max_degrees = config["landing_tilt"].value("max_degrees", 6.0f);
   _dust_burst_duration = sf::seconds(config["dust_burst"].value("duration_s", 0.5f));
   _dust_burst_peak_multiplier = config["dust_burst"].value("peak_multiplier", 4.0f);
   if (const auto fade_in_it = config.find("fade_in"); fade_in_it != config.end())
   {
      _fade_in_duration = sf::seconds(fade_in_it->value("duration_s", 0.5f));
   }

   _was_hard_landing = false;
   _jitter_elapsed = sf::Time{};
   _was_on_ground = false;
   _landing_tilt_elapsed = sf::Time{};
   _dust_burst_elapsed = sf::Time{};
   _last_valid_eye_position.reset();
   _was_eye_position_valid = false;
   _fade_in_elapsed = sf::Time{};

   if (dust_enabled)
   {
#ifdef __EMSCRIPTEN__
      auto loaded_noise_shader = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/light_noise.frag"});
      if (loaded_noise_shader.hasValue())
      {
         _noise_shader = std::make_shared<sf::Shader>(std::move(*loaded_noise_shader));
         _ul_time = _noise_shader->getUniformLocation("u_time");
         _ul_intensity = _noise_shader->getUniformLocation("u_intensity");
         _ul_flicker_speed = _noise_shader->getUniformLocation("u_flicker_speed");
         _ul_flicker_amount = _noise_shader->getUniformLocation("u_flicker_amount");
         _ul_layer_1_size = _noise_shader->getUniformLocation("u_layer_1_size");
         _ul_layer_1_speed = _noise_shader->getUniformLocation("u_layer_1_speed");
         _ul_layer_2_size = _noise_shader->getUniformLocation("u_layer_2_size");
         _ul_layer_2_speed = _noise_shader->getUniformLocation("u_layer_2_speed");
         _ul_sprite_pos_px = _noise_shader->getUniformLocation("u_sprite_pos_px");
         _ul_sprite_size_px = _noise_shader->getUniformLocation("u_sprite_size_px");

         const auto dust_callback =
            [this, dust_intensity, layer_1_size, layer_1_speed_x, layer_1_speed_y, layer_2_size, layer_2_speed_x, layer_2_speed_y](
               sf::Shader& shader, const LightSystem::LightInstance& light_instance, float elapsed_seconds
            )
         {
            if (_ul_time.hasValue())
            {
               shader.setUniform(*_ul_time, elapsed_seconds);
            }
            const float burst_factor =
               (_dust_burst_elapsed > sf::Time{})
                  ? (1.0f + (_dust_burst_peak_multiplier - 1.0f) * (_dust_burst_elapsed.asSeconds() / _dust_burst_duration.asSeconds()))
                  : 1.0f;
            if (_ul_intensity.hasValue())
            {
               shader.setUniform(*_ul_intensity, dust_intensity * burst_factor);
            }
            if (_ul_flicker_speed.hasValue())
            {
               shader.setUniform(*_ul_flicker_speed, _flicker_speed);
            }
            if (_ul_flicker_amount.hasValue())
            {
               shader.setUniform(*_ul_flicker_amount, _flicker_amount);
            }
            if (_ul_layer_1_size.hasValue())
            {
               shader.setUniform(*_ul_layer_1_size, layer_1_size);
            }
            if (_ul_layer_1_speed.hasValue())
            {
               shader.setUniform(*_ul_layer_1_speed, sf::Glsl::Vec2(layer_1_speed_x, layer_1_speed_y));
            }
            if (_ul_layer_2_size.hasValue())
            {
               shader.setUniform(*_ul_layer_2_size, layer_2_size);
            }
            if (_ul_layer_2_speed.hasValue())
            {
               shader.setUniform(*_ul_layer_2_speed, sf::Glsl::Vec2(layer_2_speed_x, layer_2_speed_y));
            }
            // the lamp-end origin is not the visual top-left; recover top-left for dust coords
            const sf::Vector2f cb_origin = light_instance._sprite->origin;
            const sf::Vector2f cb_scale = light_instance._sprite->scale;
            const sf::Vector2f cb_lamp_pos = light_instance._sprite->position;
            const sf::Vector2f cb_top_left = {cb_lamp_pos.x - cb_origin.x * cb_scale.x, cb_lamp_pos.y - cb_origin.y * cb_scale.y};
            if (_ul_sprite_pos_px.hasValue())
            {
               shader.setUniform(*_ul_sprite_pos_px, sf::Glsl::Vec2(cb_top_left));
            }
            if (_ul_sprite_size_px.hasValue())
            {
               shader.setUniform(
                  *_ul_sprite_size_px,
                  sf::Glsl::Vec2(static_cast<float>(light_instance._width_px), static_cast<float>(light_instance._height_px))
               );
            }
         };
         _player_light_left->_shader = _noise_shader;
         _player_light_left->_shader_update_callback = dust_callback;
         _player_light_right->_shader = _noise_shader;
         _player_light_right->_shader_update_callback = dust_callback;
      }
#else
      _noise_shader = std::make_shared<sf::Shader>();
      if (_noise_shader->loadFromFile("data/shaders/light_noise.frag", sf::Shader::Type::Fragment))
      {
         const auto dust_callback =
            [this, dust_intensity, layer_1_size, layer_1_speed_x, layer_1_speed_y, layer_2_size, layer_2_speed_x, layer_2_speed_y](
               sf::Shader& shader, const LightSystem::LightInstance& light_instance, float elapsed_seconds
            )
         {
            shader.setUniform("u_time", elapsed_seconds);
            const float burst_factor =
               (_dust_burst_elapsed > sf::Time::Zero)
                  ? (1.0f + (_dust_burst_peak_multiplier - 1.0f) * (_dust_burst_elapsed.asSeconds() / _dust_burst_duration.asSeconds()))
                  : 1.0f;
            shader.setUniform("u_intensity", dust_intensity * burst_factor);
            shader.setUniform("u_flicker_speed", _flicker_speed);
            shader.setUniform("u_flicker_amount", _flicker_amount);
            shader.setUniform("u_layer_1_size", layer_1_size);
            shader.setUniform("u_layer_1_speed", sf::Glsl::Vec2(layer_1_speed_x, layer_1_speed_y));
            shader.setUniform("u_layer_2_size", layer_2_size);
            shader.setUniform("u_layer_2_speed", sf::Glsl::Vec2(layer_2_speed_x, layer_2_speed_y));
            // getPosition() is the lamp-end origin, not the visual top-left; recover top-left for dust coords
            const sf::Vector2f cb_origin = light_instance._sprite->getOrigin();
            const sf::Vector2f cb_scale = light_instance._sprite->getScale();
            const sf::Vector2f cb_lamp_pos = light_instance._sprite->getPosition();
            const sf::Vector2f cb_top_left = {cb_lamp_pos.x - cb_origin.x * cb_scale.x, cb_lamp_pos.y - cb_origin.y * cb_scale.y};
            shader.setUniform("u_sprite_pos_px", sf::Glsl::Vec2(cb_top_left));
            shader.setUniform(
               "u_sprite_size_px",
               sf::Glsl::Vec2(static_cast<float>(light_instance._width_px), static_cast<float>(light_instance._height_px))
            );
         };
         _player_light_left->_shader = _noise_shader;
         _player_light_left->_shader_update_callback = dust_callback;
         _player_light_right->_shader = _noise_shader;
         _player_light_right->_shader_update_callback = dust_callback;
      }
#endif
   }

   _enabled = true;
}

void ItemLantern::onUnequipped()
{
   _enabled = false;

   auto level = LevelRegistry::getCurrent();
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
