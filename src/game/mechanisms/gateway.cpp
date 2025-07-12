#include "gateway.h"

#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/camera/camerasystem.h"
#include "game/effects/fadetransitioneffect.h"
#include "game/effects/screentransition.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

#include <iostream>

#include "gateway.h"

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
using WeakGateway = std::weak_ptr<Gateway>;
using Registry = std::unordered_multimap<std::string, WeakGateway>;

Registry gateway_registry;

void registerGateway(const std::string& id, const std::shared_ptr<Gateway>& gateway)
{
   gateway_registry.emplace(id, gateway);
}

void unregisterGateway(const std::string& id, const Gateway* raw)
{
   auto range = gateway_registry.equal_range(id);
   for (auto it = range.first; it != range.second;)
   {
      auto should_erase = false;

      if (it->second.expired())
      {
         should_erase = true;
      }
      else if (auto locked = it->second.lock(); locked.get() == raw)
      {
         should_erase = true;
      }

      if (should_erase)
      {
         it = gateway_registry.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

std::vector<std::shared_ptr<Gateway>> findGatewaysById(const std::string& id)
{
   std::vector<std::shared_ptr<Gateway>> result_it;
   auto range = gateway_registry.equal_range(id);
   for (auto it = range.first; it != range.second; ++it)
   {
      if (auto ptr = it->second.lock())
      {
         result_it.push_back(ptr);
      }
   }
   return result_it;
}
}  // namespace

namespace
{
std::unique_ptr<ScreenTransition> makeFadeTransition()
{
   auto screen_transition = std::make_unique<ScreenTransition>();
   auto fade_out = std::make_shared<FadeTransitionEffect>();
   auto fade_in = std::make_shared<FadeTransitionEffect>();
   fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
   fade_out->_speed = 2.0f;
   fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
   fade_in->_value = 1.0f;
   fade_in->_speed = 2.0f;
   screen_transition->_effect_1 = fade_out;
   screen_transition->_effect_2 = fade_in;
   screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
   screen_transition->startEffect1();

   return screen_transition;
}
}  // namespace

namespace
{
const auto registered_gateway = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("Gateway", "gateways");

   registry.registerLayerName(
      "gateways",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Gateway>(parent);
         mechanism->setup(data);
         mechanisms["gateways"]->push_back(mechanism);
      }
   );

   registry.registerObjectGroup(
      "Gateway",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Gateway>(parent);
         mechanism->setup(data);
         mechanisms["gateways"]->push_back(mechanism);
      }
   );

   return true;
}();
}  // namespace

namespace
{

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

Gateway::Gateway(GameNode* parent) : GameNode(parent)
{
   _filename = "data/sprites/gateway.psd";

   Audio::getInstance().addSample("mechanism_gateway_rotate_01.wav");
   Audio::getInstance().addSample("mechanism_gateway_extract_01.wav");
   Audio::getInstance().addSample("mechanism_gateway_warp_01.wav");
}

Gateway::~Gateway()
{
   unregisterGateway(getObjectId(), this);
}

// idea: have only 1 rotation angle, the other 3 are relative to that
// A -> angle + 0deg
// B -> angle + 90deg
// C -> angle + 180deg
// D -> angle + 270deg
//
// 1) use sfml rotation first
// 2) calculate corresponding direction vector based on the angle

void Gateway::setSidesVisible(std::array<Side, 4>& sides, bool visible)
{
   for (auto& side : sides)
   {
      side._layer->_visible = visible;
   }
}

void Gateway::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   sf::RenderStates states;

   // draw sides
   auto draw_visible = [&target, states](const auto& side)
   {
      if (side._layer->_visible)
      {
         side._layer->draw(target, states);
      }
   };

   // target.draw(_rect_shape);

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
}

void Gateway::update(const sf::Time& dt)
{
   const auto player_intersects = Player::getCurrent()->getPixelRectFloat().findIntersection(_rect).has_value();

   // activate portal when player intersects
   if (!_player_intersects && player_intersects)
   {
      _player_intersects = player_intersects;
      _state = State::Enabling;

      _activated_state._step = 0;

      Audio::getInstance().playSample({"mechanism_gateway_extract_01.wav"});

      for (auto& pa : _pa)
      {
         pa.reset();
      }
      return;
   }

   // player uses gateway
   if (_player_intersects)
   {
      if (Player::getCurrent()->getControls()->isButtonBPressed() && checkPlayerAtGateway())
      {
         use();
      }
   }

   _elapsed += dt.asSeconds();

   _origin_shape.setPosition(_origin);

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

         // lift
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
               Audio::getInstance().playSample({"mechanism_gateway_rotate_01.wav"});
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
                  Audio::getInstance().playSample({"mechanism_gateway_rotate_01.wav"});
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
                  Audio::getInstance().playSample({"mechanism_gateway_extract_01.wav"});
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

         // get back to original angle
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
               Audio::getInstance().playSample({"mechanism_gateway_warp_01.wav"});
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
            const auto time_normalized =
               std::clamp(_activated_state._elapsed_time.asSeconds() / _activated_state._fade_duration_s, 0.0f, 1.0f);
            const auto fade_out_alpha = 1.0f - time_normalized;  // pi: visible → invisible
            const auto fade_in_alpha = time_normalized;          // pa: invisible → visible

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

void Gateway::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   if (auto shared = shared_from_this())
   {
      registerGateway(getObjectId(), shared);
   }

   _rect = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   addChunks(_rect);

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   if (data._tmx_object->_properties)
   {
      const auto map = data._tmx_object->_properties->_map;

      const auto z_it = map.find("z");
      if (z_it != map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto enabled_it = map.find("enabled");
      if (enabled_it != map.end())
      {
         const auto enabled = static_cast<bool>(enabled_it->second->_value_bool.value());
         setEnabled(enabled);
      }

      _target_id = ValueReader::readValue<std::string>("target_id", map).value_or(std::format("{:08x}", std::random_device{}()));
   }

   _rect_shape.setFillColor(sf::Color(255, 255, 255, 25));
   _rect_shape.setSize(_rect.size);
   _rect_shape.setPosition(_rect.position);

   _origin_shape.setRadius(1.0f);
   _origin_shape.setFillColor(sf::Color::Red);

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

         const auto pos = sf::Vector2f{static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())} + _rect.position;
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

std::optional<sf::FloatRect> Gateway::getBoundingBoxPx()
{
   return std::nullopt;
}

bool Gateway::checkPlayerAtGateway() const
{
   const auto player_pos = Player::getCurrent()->getPixelPositionFloat();
   const auto at_door = _rect.contains(player_pos);
   return at_door;
}

void Gateway::use()
{
   if (_in_use)
   {
      return;
   }

   if (_target_id.empty())
   {
      return;
   }

   _in_use = true;

   const auto target_gateway = findOtherInstance(_target_id);
   const auto target_pos_px = target_gateway->_rect.getCenter();

   auto teleport = [&]()
   {
      {
         Player::getCurrent()->setBodyViaPixelPosition(
            target_pos_px.x + PLAYER_ACTUAL_WIDTH / 2, target_pos_px.y + DIFF_PLAYER_TILE_TO_PHYSICS
         );

         // update the camera system to point to the player position immediately
         CameraSystem::getInstance().syncNow();
      }
   };

   auto screen_transition = makeFadeTransition();
   screen_transition->_callbacks_effect_1_ended.emplace_back(teleport);
   screen_transition->_callbacks_effect_2_ended.emplace_back(
      [this]()
      {
         ScreenTransitionHandler::getInstance().pop();
         _in_use = false;
      }
   );
   ScreenTransitionHandler::getInstance().push(std::move(screen_transition));
}

std::shared_ptr<Gateway> Gateway::findOtherInstance(const std::string& gateway_id) const
{
   for (const auto& gateway : findGatewaysById(gateway_id))
   {
      if (gateway.get() != this)
      {
         return gateway;
      }
   }

   return nullptr;
}

void Gateway::setTargetId(const std::string& destination_gateway_id)
{
   _target_id = destination_gateway_id;
}

void Gateway::Side::update()
{
   const auto full_angle_sf = _angle + _angle_offset;
   sf::Vector2f pos_from_angle_and_distance_px;
   pos_from_angle_and_distance_px.x = std::cos(full_angle_sf.asRadians()) * _distance_factor;
   pos_from_angle_and_distance_px.y = std::sin(full_angle_sf.asRadians()) * _distance_factor;
   _layer->_sprite->setRotation(full_angle_sf);
   _layer->_sprite->setPosition(_pos_px + pos_from_angle_and_distance_px + _offset_px - sf::Vector2f{1.0f, 1.0f});
}

void Gateway::Side::reset()
{
   _angle = _base_angle;
   _distance_factor = 1.0f;
   _offset_px = {};
   update();
}

void Gateway::PortalState::resetTime()
{
   _elapsed_time = sf::seconds(0);
}
