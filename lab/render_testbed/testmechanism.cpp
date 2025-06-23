#include "testmechanism.h"

#include "../../src/framework/easings/easings.h"
#include "../../src/framework/image/psd.h"

#include <iostream>

TestMechanism::TestMechanism()
{
   _rectangle_.setSize({200.f, 100.f});
   _rectangle_.setFillColor(sf::Color::Red);
   _rectangle_.setPosition({540.f, 310.f});

   _origin_shape.setRadius(1.0f);
   _origin_shape.setFillColor(sf::Color::Red);

   _filename = "data/portal-test.psd";

   load();
}

namespace
{
sf::Vector2f screen_offset{100, 100};

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
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();

      try
      {
         const auto texture_size = sf::Vector2u(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
         auto texture = std::make_shared<sf::Texture>(texture_size);
         auto opacity = layer.getOpacity();

         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));
         auto sprite = std::make_shared<sf::Sprite>(*texture);

         const auto pos = sf::Vector2f{static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())} + screen_offset;
         sprite->setPosition(pos);
         sprite->setColor(sf::Color(255u, 255u, 255u, static_cast<uint8_t>(opacity)));

         tmp->_texture = texture;
         tmp->_sprite = sprite;
         tmp->_visible = layer.isVisible();
         tmp->_texture->setSmooth(true);

         _layer_stack.push_back(tmp);
         _layers[layer.getName()] = tmp;

         if (layer.getName().starts_with("pa_"))
         {
            const auto origin = sf::Vector2f{texture->getSize().x * 0.5f, texture->getSize().y * 0.5f};
            sprite->setOrigin(origin);
            sprite->setPosition(origin + pos + sf::Vector2f{0, -30});
         }
      }
      catch (...)
      {
         std::cerr << "failed to create texture: " << layer.getName();
      }
   }

   _pa[0]._layer = _layers["pa_0"];
   _pa[1]._layer = _layers["pa_1"];
   _pa[2]._layer = _layers["pa_2"];
   _pa[3]._layer = _layers["pa_3"];

   _pa[0]._pos_px = _pa[0]._layer->_sprite->getPosition();
   _pa[1]._pos_px = _pa[1]._layer->_sprite->getPosition();
   _pa[2]._pos_px = _pa[2]._layer->_sprite->getPosition();
   _pa[3]._pos_px = _pa[3]._layer->_sprite->getPosition();

   _pa[0]._angle_offset = sf::degrees(0);
   _pa[1]._angle_offset = sf::degrees(90);
   _pa[2]._angle_offset = sf::degrees(180);
   _pa[3]._angle_offset = sf::degrees(270);

   _socket_sprite = _layers["base"]->_sprite;

   _origin = _pa[0]._layer->_sprite->getOrigin();
}

void TestMechanism::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   // debug rect
   // target.draw(_rectangle_);

   sf::RenderStates states;

   // draw pa
   // std::ranges::for_each(
   //    _pa,
   //    [&target, states](const auto& pa)
   //    {
   //       //
   //       pa._layer->draw(target, states);
   //    }
   // );

   _pa[0]._layer->draw(target, states);
   _pa[1]._layer->draw(target, states);
   _pa[2]._layer->draw(target, states);
   _pa[3]._layer->draw(target, states);

   // target.draw(_origin_shape);

   target.draw(*_socket_sprite);
}

namespace
{
#include <cmath>

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
   // No-op for now
   _elapsed += dt.asSeconds();

   _origin_shape.setPosition(_origin);

   switch (_state)
   {
      case State::Disabled:
      {
         break;
      }
      case State::Enabling:
      {
         break;
      }
      case State::Enabled:
      {
         _enabled_state._elapsed_time += dt;
         constexpr auto scale_factor = 0.2f;
         constexpr auto animation_speed = 1.0f;
         const auto scaled_time = _enabled_state._elapsed_time.asSeconds() * animation_speed;
         const auto value = 0.7f * (1.0f + (std::sin(scaled_time) + std::sin(scaled_time * 3.0f) / 3.0f));

         auto index = 0;
         std::ranges::for_each(
            _pa,
            [this, &index, &value](auto& pa)
            {
               const auto full_angle_sf = pa._angle_offset; /*+ 0.1f * sf::radians(t - 0.5f)*/

               pa._distance_factor = 2.0f * (1.0f + value * 4.0f);
               pa.update();

               ++index;
            }
         );
         break;
      }
      case State::Activated:
      {
         _activated_state._elapsed_time += dt;

         // 0: lift
         // 1: rotate
         // 2: extract
         // 3: rotate a lot
         // 4: rotate to clean 90 deg (alignment with base)
         // 5: retract

         // lift
         if (_activated_state._step == 0)
         {
            float value = Easings::easeOutBounce<float>(_activated_state._elapsed_time.asSeconds());

            auto index = 0;
            std::ranges::for_each(
               _pa,
               [this, &index, &value](auto& pa)
               {
                  //
                  pa._offset_px = sf::Vector2f{0, -value * 40};
                  pa.update();
               }
            );

            if (_activated_state._elapsed_time.asSeconds() > 1.0f)
            {
               _activated_state._step++;
            }
         }

         // rotate right
         else if (_activated_state._step == 1)
         {
            if (_activated_state._elapsed_time.asSeconds() < 2.0f)
            {
               _activated_state._speed += _activated_state._acceleration * dt.asSeconds();
            }
            else
            {
               _activated_state._speed *= _activated_state._friction;

               if (_activated_state._speed < 0.0001f)
               {
                  _activated_state._step++;
                  _activated_state._speed = 0;
                  _activated_state._acceleration = -_activated_state._acceleration;
                  _activated_state.resetTime();
               }
            }

            auto index = 0;
            std::ranges::for_each(
               _pa,
               [this, &index](auto& pa)
               {
                  pa._angle += sf::radians(_activated_state._speed);
                  pa.update();
                  ++index;
               }
            );
         }

         // extract
         else if (_activated_state._step == 2)
         {
            float value = Easings::easeOutBounce<float>(_activated_state._elapsed_time.asSeconds());

            auto index = 0;
            std::ranges::for_each(
               _pa,
               [this, &index, &value](auto& pa)
               {
                  pa._distance_factor = 1.0f + value * 50;
                  pa.update();
                  ++index;
               }
            );
            if (_activated_state._elapsed_time.asSeconds() > 1.0f)
            {
               _activated_state._step++;
               _activated_state.resetTime();
            }
         }

         else if (_activated_state._step == 3)
         {
            if (_activated_state._elapsed_time.asSeconds() < 3.0f)
            {
               _activated_state._speed += _activated_state._acceleration * dt.asSeconds();
            }
            else
            {
               _activated_state._speed *= _activated_state._friction;

               if (_activated_state._speed > -0.0001f)
               {
                  _activated_state._step++;
                  _activated_state._speed = 0;
                  _activated_state.resetTime();
               }
            }

            auto index = 0;
            std::ranges::for_each(
               _pa,
               [this, &index](auto& pa)
               {
                  pa._angle += sf::radians(_activated_state._speed);
                  pa.update();
                  ++index;
               }
            );
         }

         // rotate a lot
         else if (_activated_state._step == 999)
         {
            if (_activated_state._elapsed_time.asSeconds() < 10.0f)
            {
               _activated_state._speed += _activated_state._acceleration * dt.asSeconds();
            }
            else
            {
               _activated_state._speed *= _activated_state._friction;

               if (_activated_state._speed > -0.0001f)
               {
                  _activated_state._step++;
                  _activated_state.resetTime();
               }
            }

            auto index = 0;
            std::ranges::for_each(
               _pa,
               [this, &index](auto& pa)
               {
                  pa._angle += sf::radians(_activated_state._speed);
                  pa.update();
                  ++index;
               }
            );
         }
         else if (_activated_state._step == 4)
         {
            float duration = 1.0f;

            if (!_activated_state._has_target_angle)
            {
               float current = _pa.front()._angle.asRadians();  // assume all are the same
               float quarter_turn = std::numbers::pi_v<float> / 2.0f;
               float snapped = std::round(current / quarter_turn) * quarter_turn;

               _activated_state._angle_start = sf::radians(current);
               _activated_state._angle_target = sf::radians(snapped);
               _activated_state._has_target_angle = true;
            }

            float t = std::clamp(_activated_state._elapsed_time.asSeconds() / duration, 0.0f, 1.0f);
            float eased = Easings::easeOutCubic(t);
            sf::Angle angle = _activated_state._angle_start + (_activated_state._angle_target - _activated_state._angle_start) * eased;

            for (auto& pa : _pa)
            {
               pa._angle = angle;
               pa.update();
            }

            if (t >= 1.0f)
            {
               _activated_state._step++;
               _activated_state._has_target_angle = false;
               _activated_state.resetTime();
            }

            break;
         }

         else if (_activated_state._step == 5)
         {
            float duration = 1.0f;
            float t = std::clamp(_activated_state._elapsed_time.asSeconds() / duration, 0.0f, 1.0f);
            float eased = Easings::easeInOutQuad(1.0f - t);  // reverse easing for smooth return

            for (auto& pa : _pa)
            {
               pa._offset_px = sf::Vector2f{0.0f, -eased * 40.0f};
               pa._distance_factor = 1.0f + eased * 50.0f;
               pa.update();
            }

            if (t >= 1.0f)
            {
               _activated_state._step = 0;
               _activated_state.resetTime();
            }

            break;
         }

         break;
      }
   }
}
