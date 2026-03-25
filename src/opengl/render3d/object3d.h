#pragma once

#include <memory>

#include "opengl/glm/glm.hpp"
#include "opengl/glm/gtc/matrix_transform.hpp"
#include "opengl/glslprogram.h"

/// \brief abstract 3d scene object with transform and render/update hooks.
class Object3D
{
public:
   Object3D() = default;
   virtual ~Object3D() = default;

   /// \brief advances per-object simulation state.
   /// \param deltaTime elapsed time in seconds.
   virtual void update(float deltaTime) = 0;

   /// \brief renders the object using provided shader and camera matrices.
   /// \param shader shader program to configure and use.
   /// \param view_matrix current camera view matrix.
   /// \param projection_matrix current camera projection matrix.
   virtual void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) = 0;

   /// \brief sets object world position.
   /// \param position new translation vector.
   virtual void setPosition(const glm::vec3& position);

   /// \brief returns object world position.
   /// \return current translation vector.
   virtual glm::vec3 getPosition() const;

   /// \brief sets object Euler rotation values.
   /// \param rotation rotation components in radians.
   virtual void setRotation(const glm::vec3& rotation);

   /// \brief returns object Euler rotation values.
   /// \return current rotation components in radians.
   virtual glm::vec3 getRotation() const;

   /// \brief sets object scale factors.
   /// \param scale scale factor per axis.
   virtual void setScale(const glm::vec3& scale);

   /// \brief returns object scale factors.
   /// \return current scale vector.
   virtual glm::vec3 getScale() const;

protected:
   glm::vec3 _position{0.0f, 0.0f, 0.0f};
   glm::vec3 _rotation{0.0f, 0.0f, 0.0f};
   glm::vec3 _scale{1.0f, 1.0f, 1.0f};
};
