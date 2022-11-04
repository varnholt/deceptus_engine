#include "rotatingblade.h"

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

   // exit early if velocity is under a certain threshold
   const auto movement_delta = dt.asSeconds() * _velocity * _settings._movement_speed;
   _path_interpolation.updateTime(movement_delta);

   _angle += dt.asSeconds() * _velocity * _direction * _settings._blade_rotation_speed;
   const auto pos = _path_interpolation.computePosition(_path_interpolation.getTime());

   _sprite.setRotation(_angle);
   _sprite.setPosition(pos);

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

std::optional<sf::FloatRect> RotatingBlade::getBoundingBoxPx()
{
   return _rectangle;
}

const sf::FloatRect& RotatingBlade::getPixelRect() const
{
   return _rectangle;
}
