#include "object3d.h"

void Object3D::setPosition(const glm::vec3& position)
{
   _position = position;
}

void Object3D::setRotation(const glm::vec3& rotation)
{
   _rotation = rotation;
}

void Object3D::setScale(const glm::vec3& scale)
{
   _scale = scale;
}

glm::vec3 Object3D::getScale() const
{
   return _scale;
}

glm::vec3 Object3D::getRotation() const
{
   return _rotation;
}

glm::vec3 Object3D::getPosition() const
{
   return _position;
}
