#pragma once

#include <memory>

#include "opengl/glm/glm.hpp"
#include "opengl/glm/gtc/matrix_transform.hpp"
#include "opengl/glslprogram.h"

class Object3D
{
public:
   Object3D() = default;
   virtual ~Object3D() = default;

   virtual void update(float deltaTime) = 0;
   virtual void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) = 0;

   virtual void setPosition(const glm::vec3& position);
   virtual glm::vec3 getPosition() const;

   virtual void setRotation(const glm::vec3& rotation);
   virtual glm::vec3 getRotation() const;

   virtual void setScale(const glm::vec3& scale);
   virtual glm::vec3 getScale() const;

protected:
   glm::vec3 _position{0.0f, 0.0f, 0.0f};
   glm::vec3 _rotation{0.0f, 0.0f, 0.0f};
   glm::vec3 _scale{1.0f, 1.0f, 1.0f};
};
