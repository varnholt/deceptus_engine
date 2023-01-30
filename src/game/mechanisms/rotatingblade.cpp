#include "rotatingblade.h"

#include "audio.h"
#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxpolygon.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "game/debugdraw.h"
#include "game/player/player.h"
#include "game/texturepool.h"

// #define DEBUG_INTERSECTION

RotatingBlade::RotatingBlade(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(RotatingBlade).name());

   setZ(30);

   _texture_map = TexturePool::getInstance().get("data/sprites/enemy_rotating_blade.png");
   _sprite.setTexture(*_texture_map.get());
   _sprite.setOrigin(_texture_map->getSize().x * 0.5f, _texture_map->getSize().y * 0.5f);

   _audio_range = AudioRange{600.0f, 0.0f, 100.0f, 1.0f};
   _has_audio = true;
}

RotatingBlade::~RotatingBlade()
{
   if (_sample_enabled.has_value())
   {
      Audio::getInstance().stopSample(_sample_enabled.value());
   }

   if (_sample_accelerate.has_value())
   {
      Audio::getInstance().stopSample(_sample_accelerate.value());
   }

   if (_sample_decelerate.has_value())
   {
      Audio::getInstance().stopSample(_sample_decelerate.value());
   }
}

void RotatingBlade::setup(const GameDeserializeData& data)
{
   if (!data._tmx_object->_polygon && !data._tmx_object->_polyline)
   {
      Log::Error() << "the tmx object is neither a polygon or polyline";
      return;
   }

   _path = data._tmx_object->_polygon ? data._tmx_object->_polygon->_polyline : data._tmx_object->_polyline->_polyline;
   _path.push_back(_path.at(0));  // close path
   _path_type = data._tmx_object->_polygon ? PathType::Polygon : PathType::Polyline;

   std::transform(
      _path.begin(),
      _path.end(),
      _path.begin(),
      [data](auto& vec) {
         return vec + sf::Vector2f{data._tmx_object->_x_px, data._tmx_object->_y_px};
      }
   );

   _path_interpolation.addKeys(_path);

   // collision rect for lever
   _rectangle = {data._tmx_object->_x_px, data._tmx_object->_y_px, 64, 64};

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         setEnabled(enabled_it->second->_value_bool.value());
      }

      const auto blade_acceleration_it = data._tmx_object->_properties->_map.find("blade_acceleration");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_acceleration = blade_acceleration_it->second->_value_float.value();
      }

      const auto blade_deceleration_it = data._tmx_object->_properties->_map.find("blade_deceleration");
      if (blade_deceleration_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_deceleration = blade_deceleration_it->second->_value_float.value();
      }

      const auto _blade_rotation_speed_it = data._tmx_object->_properties->_map.find("blade_rotation_speed");
      if (_blade_rotation_speed_it != data._tmx_object->_properties->_map.end())
      {
         _settings._blade_rotation_speed = _blade_rotation_speed_it->second->_value_float.value();
      }

      const auto movement_speed_it = data._tmx_object->_properties->_map.find("movement_speed");
      if (movement_speed_it != data._tmx_object->_properties->_map.end())
      {
         _settings._movement_speed = movement_speed_it->second->_value_float.value();
      }
   }
}

void RotatingBlade::updateAudio()
{
   if (!isAudioEnabled())
   {
      // stop whatever is playing if audio is disabled
      if (_sample_enabled.has_value())
      {
         Audio::getInstance().stopSample(_sample_enabled.value());
      }

      if (_sample_accelerate.has_value())
      {
         Audio::getInstance().stopSample(_sample_accelerate.value());
      }

      if (_sample_decelerate.has_value())
      {
         Audio::getInstance().stopSample(_sample_decelerate.value());
      }

      return;
   }

   static constexpr auto eps_enabled_on = 0.5f;
   static constexpr auto eps_enabled_off = 0.05f;
   static constexpr auto eps_accelerate_off = 0.05f;

   // blades are accelerating until rotating at regular speed
   if (_enabled)
   {
      if (_velocity > 1.0f - eps_enabled_on)
      {
         // play regular sample
         if (!_sample_enabled.has_value())
         {
            _sample_enabled = Audio::getInstance().playSample({"mechanism_rotating_blade_enabled.wav", 1.0f, true});
         }
         else
         {
            if (_velocity > 1.0f - eps_accelerate_off)
            {
               _sample_accelerate.reset();
            }

            Audio::getInstance().setPosition(_sample_enabled.value(), _pos);
         }
      }
      else
      {
         // play acceleration sample
         if (!_sample_accelerate.has_value())
         {
            _sample_accelerate = Audio::getInstance().playSample({"mechanism_rotating_blade_accelerate.wav"});
         }
         else
         {
            Audio::getInstance().setPosition(_sample_accelerate.value(), _pos);
         }
      }
   }

   // blades are slowing down until they're fully stopped
   else
   {
      if (_velocity < 0.0 + eps_enabled_off)
      {
         // stop decelerate sample
         if (_sample_decelerate.has_value())
         {
            Audio::getInstance().stopSample(_sample_decelerate.value());
            _sample_decelerate.reset();
         }
      }
      else
      {
         // play deceleration sample
         if (!_sample_decelerate.has_value())
         {
            _sample_decelerate = Audio::getInstance().playSample({"mechanism_rotating_blade_decelerate.wav"});
         }
         else
         {
            Audio::getInstance().setPosition(_sample_decelerate.value(), _pos);
         }

         // stop playing enabled sample if it's been playing before
         if (_sample_enabled.has_value())
         {
            Audio::getInstance().stopSample(_sample_enabled.value());
         }
      }
   }
}

void RotatingBlade::update(const sf::Time& dt)
{
   if (_enabled)
   {
      _velocity = std::min<float>(1.0f, _velocity + _settings._blade_acceleration);
   }
   else
   {
      _velocity = std::max<float>(0.0f, _velocity - _settings._blade_deceleration);
   }

   // update position and rotation along path
   const auto movement_delta = dt.asSeconds() * _velocity * _settings._movement_speed;
   _path_interpolation.updateTime(movement_delta);
   _angle += dt.asSeconds() * _velocity * _direction * _settings._blade_rotation_speed;
   _pos = _path_interpolation.computePosition(_path_interpolation.getTime());
   _sprite.setRotation(_angle);
   _sprite.setPosition(_pos);

   updateAudio();

   // kill player if he moves into the blade's radius
   sf::Vector2i blade_position{_sprite.getPosition()};
   const auto blade_radius = static_cast<int32_t>(_texture_map->getSize().x * 0.5f);
   if (SfmlMath::intersectCircleRect(blade_position, blade_radius, Player::getCurrent()->getPixelRectInt()))
   {
      if (_velocity > 0.3f)
      {
         Player::getCurrent()->damage(100);
      }
   }
}

void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);

#ifdef DEBUG_INTERSECTION
   sf::Vector2i sprite_center{_sprite.getPosition()};
   const auto blade_radius = static_cast<int32_t>(_texture_map->getSize().x * 0.5f);

   b2Color color{1.0f, 1.0f, 1.0f};
   if (SfmlMath::intersectCircleRect(sprite_center, blade_radius, Player::getCurrent()->getPlayerPixelRect()))
   {
      color = b2Color{1.0f, 0.0f, 0.0f};
   }

   DebugDraw::drawCircle(target, _sprite.getPosition(), _sprite.getOrigin().x, color);
#endif
}

void RotatingBlade::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
}

void RotatingBlade::setAudioEnabled(bool enabled)
{
   GameMechanism::setAudioEnabled(enabled);
}

void RotatingBlade::setVolume(float volume)
{
   GameMechanism::setVolume(volume);

   if (_sample_enabled.has_value())
   {
      Audio::getInstance().setVolume(_sample_enabled.value(), volume);
   }

   if (_sample_accelerate.has_value())
   {
      Audio::getInstance().setVolume(_sample_accelerate.value(), volume);
   }

   if (_sample_decelerate.has_value())
   {
      Audio::getInstance().setVolume(_sample_decelerate.value(), volume);
   }
}

std::optional<sf::FloatRect> RotatingBlade::getBoundingBoxPx()
{
   return _rectangle;
}

const sf::FloatRect& RotatingBlade::getPixelRect() const
{
   return _rectangle;
}
