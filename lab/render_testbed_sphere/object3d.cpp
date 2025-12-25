#include "object3d.h"

void Object3D::setPosition(const vec3& position)
{
   _position = position;
}

void Object3D::setRotation(const vec3& rotation)
{
   _rotation = rotation;
}

void Object3D::setScale(const vec3& scale)
{
   _scale = scale;
}

vec3 Object3D::getScale() const
{
   return _scale;
}

vec3 Object3D::getRotation() const
{
   return _rotation;
}

vec3 Object3D::getPosition() const
{
   return _position;
}
