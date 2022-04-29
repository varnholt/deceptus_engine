#include "rotatingblade.h"

#include "framework/tmxparser/tmxpolyline.h"
#include "game/texturepool.h"


namespace
{
}


RotatingBlade::RotatingBlade(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(RotatingBlade).name());

   _texture_map = TexturePool::getInstance().get("data/sprites/enemy_rotating_blade.png");
   _sprite.setTexture(*_texture_map.get());
}


void RotatingBlade::setup(const GameDeserializeData& data)
{
   if (!data._tmx_object->_polyline)
   {
      return;
   }

   _path = data._tmx_object->_polyline->_polyline;
   _path_interpolation.addKeys(_path);
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

   _angle += dt.asSeconds() * _velocity * _direction * _settings._blade_rotation_speed;

   _sprite.setRotation(_angle);

   _path_interpolation.updateTime(dt.asSeconds());
   const auto pos = _path_interpolation.computePosition(_path_interpolation.getTime());
   _sprite.setPosition(pos);
}


void RotatingBlade::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   target.draw(_sprite);
}


void RotatingBlade::setEnabled(bool /*enabled*/)
{

}
