#pragma once

#include <array>
#include "opengl/glm/glm.hpp"

class Camera3D
{
public:
   Camera3D() = default;

   void initialize(int32_t w, int32_t h);
   void initialize(int32_t w, int32_t h, float nearPlane, float farPlane);

   float getFOV() const;
   void setFOV(float fov);

   const glm::mat4& getProjectionMatrix() const;
   void setProjectionMatrix(const glm::mat4& projection_matrix);

   glm::mat4 getViewMatrixCopy() const;
   const glm::mat4& getViewMatrix() const;
   void setViewMatrix(const glm::mat4& view_matrix);

   const glm::vec3& getPosition() const;
   void setPosition(const glm::vec3& Camera3D_position);

   const glm::vec3& getLookAtPoint() const;
   void setLookAtPoint(const glm::vec3& look_at_point);

   void updateViewMatrix();

   const std::array<int32_t, 2>& getScreenDimensions() const;

private:
   glm::mat4 _projection_matrix;
   glm::mat4 _view_matrix;
   glm::vec3 _position;
   glm::vec3 _look_at_point{0.0f, 0.0f, 0.0f};

   std::array<int32_t, 2> _screen_dimensions;

   float _fov{70.0f};
   float _near_plane{0.3f};
   float _far_plane{1000.0f};
   void updateProjectionMatrix();
};
