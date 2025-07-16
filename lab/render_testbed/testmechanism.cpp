#include "testmechanism.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include "../../src/framework/easings/easings.h"
#include "../../src/framework/image/psd.h"

#include <iostream>

namespace
{
sf::Vector2f screen_offset{400, 300};

std::shared_ptr<sf::Texture> createRotatedTexture(const sf::Texture& original, const sf::Angle& angle)
{
   const auto size_px = original.getSize();
   const auto width_px = static_cast<float>(size_px.x);
   const auto height_px = static_cast<float>(size_px.y);

   sf::RenderTexture render_texture(size_px);
   render_texture.clear(sf::Color::Transparent);

   sf::Sprite sprite(original);
   sprite.setOrigin({width_px / 2.0f, height_px / 2.0f});
   sprite.setRotation(angle);
   sprite.setPosition({width_px / 2.0f, height_px / 2.0f});

   render_texture.draw(sprite);
   render_texture.display();

   auto rotated = std::make_shared<sf::Texture>(render_texture.getTexture());
   return rotated;
}

}  // namespace

TestMechanism::TestMechanism()
{
   _rectangle_.setSize({200.f, 200.f});
   _rectangle_.setFillColor(sf::Color(255, 255, 255, 25));
   _rectangle_.setPosition(screen_offset);

   _origin_shape.setRadius(1.0f);
   _origin_shape.setFillColor(sf::Color::Red);

   _filename = "data/portal-test.psd";

   // shader
   if (!_shader.loadFromFile("data/void_standalone.frag", sf::Shader::Type::Fragment))
   {
      std::cout << "failed to load shader" << std::endl;
   }

   if (!noise_texture.loadFromFile("data/noise.png"))
   {
      std::cerr << "Failed to load noise texture" << std::endl;
   }

   noise_texture.setRepeated(true);
   noise_texture.setSmooth(true);  // optional, depending on style

   _shader.setUniform("iChannel0", noise_texture);

   _shader_texture = std::make_unique<sf::RenderTexture>(sf::Vector2u(200u, 200u));
   _shader_sprite = std::make_unique<sf::Sprite>(_shader_texture->getTexture());
   _shader_sprite->setPosition(screen_offset);  // same as your old rect

   load();
}

// idea: have only 1 rotation angle, the other 3 are relative to that
// A -> angle + 0deg
// B -> angle + 90deg
// C -> angle + 180deg
// D -> angle + 270deg
//
// 1) use sfml rotation first
// 2) calculate corresponding direction vector based on the angle

void TestMechanism::load()
{
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load(_filename);

   int32_t pa_index = 0;
   for (const auto& layer : psd.getLayers())
   {
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();

      try
      {
         const auto texture_size = sf::Vector2u(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
         auto opacity = layer.getOpacity();

         auto texture = std::make_shared<sf::Texture>(texture_size);
         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));

         std::shared_ptr<sf::Sprite> sprite;

         // rotate texture if this is a pa_ or pi_ layer
         if (layer.getName().starts_with("pa_") || layer.getName().starts_with("pi_"))
         {
            texture->setSmooth(true);
            texture = createRotatedTexture(*texture, -_base_angle);  // rotate ccw
         }

         sprite = std::make_shared<sf::Sprite>(*texture);

         const auto pos = sf::Vector2f{static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())} + screen_offset;
         sprite->setPosition(pos);
         sprite->setColor(sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity)));

         tmp->_texture = texture;
         tmp->_sprite = sprite;
         tmp->_visible = layer.isVisible();

         _layer_stack.push_back(tmp);
         _layers[layer.getName()] = tmp;

         if (layer.getName().starts_with("pa_") || layer.getName().starts_with("pi_"))
         {
            const auto origin = sf::Vector2f{texture->getSize().x * 0.5f, texture->getSize().y * 0.5f};
            sprite->setOrigin(origin);
            sprite->setPosition(origin + pos + sf::Vector2f{0, 0});
         }
      }
      catch (...)
      {
         std::cerr << "failed to create texture: " << layer.getName();
      }
   }

   for (int i = 0; i < 4; ++i)
   {
      _pa[i]._layer = _layers[std::format("pa_{}", i)];
      _pa[i]._pos_px = _pa[i]._layer->_sprite->getPosition();
      _pa[i]._angle_offset = sf::degrees(90 * i);

      _pi[i]._layer = _layers[std::format("pi_{}", i)];
      _pi[i]._pos_px = _pi[i]._layer->_sprite->getPosition();
      _pi[i]._angle_offset = sf::degrees(90 * i);
   }

   _sprite_socket = _layers["base"]->_sprite;
   _layer_background_inactive = _layers["background_inactive"];
   _layer_background_active = _layers["background_active"];

   _origin = _pa[0]._layer->_sprite->getOrigin();
}

void TestMechanism::drawEditor()
{
   ImGui::Begin("Settings");

   ImGui::Text("Activating State");
   ImGui::SliderFloat("acceleration", &_activated_state._acceleration, -0.01f, 0.01f);
   ImGui::SliderFloat("friction", &_activated_state._friction, 0.9f, 1.0f);
   ImGui::SliderInt("rise_height_px", &_activated_state._rise_height_px, 0, 255);
   ImGui::SliderInt("extend_distance_px", &_activated_state._extend_distance_px, 0, 255);
   ImGui::SliderFloat("spinback_duration_s", &_activated_state._spinback_duration_s, 0.1f, 10.0f);
   ImGui::SliderFloat("retract_duration_s", &_activated_state._retract_duration_s, 0.1f, 10.0f);
   ImGui::SliderFloat("rotate_right_duration_s", &_activated_state._rotate_right_duration_s, 0.1f, 10.0f);
   ImGui::SliderFloat("rotate_left_duration_s", &_activated_state._rotate_left_duration_s, 0.1f, 10.0f);
   ImGui::SliderFloat("rotate_speed_max", &_activated_state._rotate_speed_max, 0.0002f, 0.02f);

   ImGui::Separator();

   ImGui::Text("Enabled State");
   ImGui::SliderFloat("frequency", &_enabled_state._frequency, 0.1f, 10.0f);
   ImGui::SliderFloat("amplitude", &_enabled_state._amplitude, 0.0f, 10.0f);
   ImGui::SliderFloat("offset", &_enabled_state._offset, 0.0f, 5.0f);
   ImGui::SliderFloat("irregularity", &_enabled_state._irregularity, 0.0f, 10.0f);

   ImGui::Separator();
   ImGui::Text("Plasma Shader");
   ImGui::SliderFloat("Plasma Radius", &_radius, 0.0f, 500.0f);
   ImGui::SliderFloat("Plasma Alpha", &_alpha, 0.0f, 1.0f);
   ImGui::End();
}

void TestMechanism::setSidesVisible(std::array<Side, 4>& sides, bool visible)
{
   for (auto& side : sides)
   {
      side._layer->_visible = visible;
   }
}

void TestMechanism::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   sf::RenderStates states;
   drawEditor();

   // Save current view (likely the default or GUI-compatible)
   const sf::View original_view = target.getView();
   sf::View pixel_view = original_view;

   // pixel view
   constexpr auto zoom = 0.5f;
   pixel_view.zoom(zoom);
   target.setView(pixel_view);

   // render shader
   const auto r = std::max(_pi.at(0)._distance_factor, _pa.at(0)._distance_factor);
   const auto d = std::max(abs(_pi.at(0)._offset_px.y), abs(_pa.at(0)._offset_px.y));
   const auto pos = screen_offset + sf::Vector2f{-5, -50} - sf::Vector2f{0, d * 0.6f};

   const auto radius = _radius + 2.0f * r;

   if (_state != State::Disabled)
   {
      _shader_texture->clear(sf::Color::Transparent);

      _shader.setUniform("time", _elapsed);
      _shader.setUniform("alpha", _alpha);
      _shader.setUniform("radius_factor", _radius);
      _shader.setUniform("resolution", sf::Vector2f{200, 200});

      sf::RenderStates shader_state;
      shader_state.shader = &_shader;

      sf::RectangleShape quad(sf::Vector2f{200.f, 200.f});
      quad.setFillColor(sf::Color::White);  // needed to apply the shader
      _shader_texture->draw(quad, shader_state);

      _shader_texture->display();

      _shader_sprite->setPosition(pos);
      target.draw(*_shader_sprite);  // draw final result onto main target
   }

   // draw sides
   auto draw_visible = [&target, states](const auto& side)
   {
      if (side._layer->_visible)
      {
         side._layer->draw(target, states);
      }
   };

   // target.draw(_rectangle_);

   if (_layer_background_inactive->_visible)
   {
      target.draw(*_layer_background_inactive);
   }

   if (_layer_background_active->_visible)
   {
      target.draw(*_layer_background_active);
   }

   target.draw(*_sprite_socket);

   std::ranges::for_each(_pa, draw_visible);
   std::ranges::for_each(_pi, draw_visible);

   // debug rect
   // target.draw(_origin_shape);

   target.setView(original_view);
}

namespace
{
#include <cmath>

float f(double x, double sigma = 0.8)
{
   double x_mod = std::fmod(x, 5.0);  // Repeat every 5 units
   return std::exp(-std::pow(x_mod, 2) / (2.0 * sigma * sigma));
}

}  // namespace

// tanh(sin((x-0.5)*PI)*4-2.5)*0.5+ 6
// smoothstep(-0.5, 0.5, sin(x+t))
// sin(x) + sin(x*3)/4
// const auto t = 0.5f * (1.0f + std::sin(x));
// const auto t = f(x);
// const float t = std::tanh(std::sin((x - 0.5) * std::numbers::pi) * 4 - 2.5) * 0.5 + 0.5;

void TestMechanism::update(const sf::Time& dt)
{
   _elapsed += dt.asSeconds();

   _origin_shape.setPosition(screen_offset + _origin);

   switch (_state)
   {
      case State::Disabled:
      {
         setSidesVisible(_pa, false);
         setSidesVisible(_pi, true);
         _layer_background_active->hide();
         _layer_background_inactive->show();

         std::ranges::for_each(
            _pi,
            [this](auto& side)
            {
               side.reset();
               side.update();
            }
         );
         break;
      }

      case State::Enabled:
      {
         setSidesVisible(_pa, true);
         setSidesVisible(_pi, false);
         _layer_background_active->show();
         _layer_background_inactive->hide();

         _enabled_state._elapsed_time += dt;
         constexpr auto animation_speed = 1.0f;
         const auto scaled_time = _enabled_state._elapsed_time.asSeconds() * animation_speed;
         const auto target_distance =
            2.0f * (_enabled_state._offset +
                    _enabled_state._amplitude * (1.0f + (std::sin(scaled_time) + std::sin(scaled_time * _enabled_state._irregularity) /
                                                                                    _enabled_state._irregularity)));

         // interpolate from last position given in activation
         constexpr auto interpolation_duration = 2.0f;
         const auto interpolating = _enabled_state._elapsed_time.asSeconds() < interpolation_duration;
         const auto normalized_time = std::clamp(_enabled_state._elapsed_time.asSeconds() / interpolation_duration, 0.0f, 1.0f);

         std::ranges::for_each(
            _pa,
            [this, &target_distance, interpolating, normalized_time](auto& pa)
            {
               pa._distance_factor =
                  interpolating ? std::lerp(_enabled_state._distances_when_activated, target_distance, normalized_time) : target_distance;

               pa.update();
            }
         );
         break;
      }

      case State::Enabling:
      {
         _layer_background_active->hide();
         _layer_background_inactive->hide();

         _activated_state._elapsed_time += dt;

         // 0: lift and extract
         // 1: rotate
         // 2: rotate to clean 90 deg (alignment with base)
         // 3: retract

         // lift and extract
         if (_activated_state._step == 0)
         {
            setSidesVisible(_pa, false);
            setSidesVisible(_pi, true);

            const auto lift_factor = Easings::easeOutBounce<float>(_activated_state._elapsed_time.asSeconds());
            const auto distance_factor = Easings::easeOutBounce<float>(_activated_state._elapsed_time.asSeconds());

            std::ranges::for_each(
               _pi,
               [this, &lift_factor, &distance_factor](auto& side)
               {
                  side._distance_factor = 1.0f + distance_factor * _activated_state._extend_distance_px;
                  side._offset_px = sf::Vector2f{0, -lift_factor * _activated_state._rise_height_px};
                  side.update();
               }
            );

            if (_activated_state._elapsed_time.asSeconds() > 1.0f)
            {
               _activated_state.resetTime();
               _activated_state._step++;
            }
         }

         // rotate right
         else if (_activated_state._step == 1)
         {
            if (_activated_state._elapsed_time.asSeconds() < _activated_state._rotate_right_duration_s)
            {
               _activated_state._speed += _activated_state._acceleration * dt.asSeconds();
               _activated_state._speed = std::clamp(_activated_state._speed, 0.0f, _activated_state._rotate_speed_max);
            }
            else
            {
               _activated_state._speed *= _activated_state._friction;

               if (_activated_state._speed < 0.0001f)
               {
                  _activated_state._speed = 0;

                  _activated_state._step++;
                  _activated_state.resetTime();
               }
            }

            auto index = 0;
            std::ranges::for_each(
               _pi,
               [this, &index](auto& side)
               {
                  side._angle += sf::radians(_activated_state._speed);
                  side.update();
                  ++index;
               }
            );
         }

         // rotate left
         else if (_activated_state._step == 2)
         {
            if (_activated_state._elapsed_time.asSeconds() < _activated_state._rotate_left_duration_s)
            {
               _activated_state._speed -= _activated_state._acceleration * dt.asSeconds();
               _activated_state._speed = std::clamp(_activated_state._speed, -_activated_state._rotate_speed_max, 0.0f);
            }
            else
            {
               _activated_state._speed *= _activated_state._friction;

               if (_activated_state._speed > -0.0001f)
               {
                  _activated_state._speed = 0;

                  _activated_state._step++;
                  _activated_state.resetTime();
               }
            }

            auto index = 0;
            std::ranges::for_each(
               _pi,
               [this, &index](auto& side)
               {
                  side._angle += sf::radians(_activated_state._speed);
                  side.update();
                  ++index;
               }
            );
         }

         else if (_activated_state._step == 3)
         {
            if (!_activated_state._has_target_angle)
            {
               const auto current_angle = _pi.front()._angle.asRadians();  // assume all are the same
               const auto quarter_turn = std::numbers::pi_v<float> / 2.0f;
               const auto snapped_angle = _base_angle.asRadians() + std::round(current_angle / quarter_turn) * quarter_turn;

               _activated_state._angle_start = sf::radians(current_angle);
               _activated_state._angle_target = sf::radians(snapped_angle);
               _activated_state._has_target_angle = true;
            }

            const auto normalized_time =
               std::clamp(_activated_state._elapsed_time.asSeconds() / _activated_state._spinback_duration_s, 0.0f, 1.0f);

            const auto eased = Easings::easeOutCubic(normalized_time);

            const auto angle = _activated_state._angle_start + (_activated_state._angle_target - _activated_state._angle_start) * eased;

            for (auto& side : _pi)
            {
               side._angle = angle;
               side.update();
            }

            if (normalized_time >= 1.0f)
            {
               _activated_state._step++;
               _activated_state._has_target_angle = false;
               _activated_state.resetTime();
            }

            break;
         }

         else if (_activated_state._step == 4)
         {
            float time_normalized =
               std::clamp(_activated_state._elapsed_time.asSeconds() / _activated_state._retract_duration_s, 0.0f, 1.0f);
            float eased = Easings::easeInOutQuad(1.0f - time_normalized);  // reverse easing

            for (auto& side : _pi)
            {
               side._offset_px = sf::Vector2f{0.0f, -eased * _activated_state._rise_height_px};
               side._distance_factor = 1.0f + eased * _activated_state._extend_distance_px;
               side.update();
            }

            if (time_normalized >= 1.0f)
            {
               _activated_state._step = 5;
               _activated_state.resetTime();
            }

            break;
         }

         else if (_activated_state._step == 5)
         {
            float time_normalized = std::clamp(_activated_state._elapsed_time.asSeconds() / _activated_state._fade_duration_s, 0.0f, 1.0f);
            float fade_out_alpha = 1.0f - time_normalized;  // pi: visible → invisible
            float fade_in_alpha = time_normalized;          // pa: invisible → visible

            setSidesVisible(_pa, true);
            setSidesVisible(_pi, true);

            _layer_background_active->show();
            _layer_background_inactive->show();

            for (int i = 0; i < 4; ++i)
            {
               auto& pi_layer = _pi[i]._layer->_sprite;
               auto& pa_layer = _pa[i]._layer->_sprite;

               auto pi_color = pi_layer->getColor();
               auto pa_color = pa_layer->getColor();

               pi_color.a = static_cast<uint8_t>(255 * fade_out_alpha);
               pa_color.a = static_cast<uint8_t>(255 * fade_in_alpha);

               pi_layer->setColor(pi_color);
               pa_layer->setColor(pa_color);
               _layer_background_active->_sprite->setColor(pa_color);
               _layer_background_inactive->_sprite->setColor(pi_color);
            }

            std::ranges::for_each(_pi, [](auto& s) { s.update(); });
            std::ranges::for_each(_pa, [](auto& s) { s.update(); });

            if (time_normalized >= 1.0f)
            {
               for (auto& side : _pi)
               {
                  side.reset();
                  side._layer->_sprite->setColor(sf::Color(255, 255, 255, 255));
               }

               for (auto& side : _pa)
               {
                  side._layer->_sprite->setColor(sf::Color(255, 255, 255, 255));
               }

               _activated_state._step = 0;
               _activated_state.resetTime();

               // go to enabled state after activation
               _state = State::Enabled;
               _enabled_state.resetTime();
               _enabled_state._distances_when_activated = _pa[0]._distance_factor;
            }
         }

         break;
      }
   }
}

void TestMechanism::chooseNextState()
{
   switch (_state)
   {
      case State::Disabled:
      {
         _state = State::Enabling;
         break;
      }
      case State::Enabling:
      {
         _state = State::Enabled;
         break;
      }
      case State::Enabled:
      {
         _state = State::Disabled;
         break;
      }
   }

   _activated_state._step = 0;

   for (auto& pa : _pa)
   {
      pa.reset();
   }
}
