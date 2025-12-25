#include "camera3d.h"

// glm
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

// Camera3D& Camera3D::getInstance()
// {
//    static Camera3D __Camera3D;
//    return __Camera3D;
// }

void Camera3D::initialize(int32_t w, int32_t h)
{
   initialize(w, h, 0.3f, 1000.0f);  // Use default values
}

void Camera3D::updateProjectionMatrix()
{
   _projection_matrix = glm::perspective(
      glm::radians(_fov), static_cast<float>(_screen_dimensions[0]) / static_cast<float>(_screen_dimensions[1]), _near_plane, _far_plane
   );
}

void Camera3D::initialize(int32_t w, int32_t h, float nearPlane, float farPlane)
{
   _near_plane = nearPlane;
   _far_plane = farPlane;
   _screen_dimensions[0] = w;
   _screen_dimensions[1] = h;

   updateProjectionMatrix();
   updateViewMatrix();
}

const glm::mat4& Camera3D::getProjectionMatrix() const
{
   return _projection_matrix;
}

void Camera3D::setProjectionMatrix(const glm::mat4& projection_matrix)
{
   _projection_matrix = projection_matrix;
}

const glm::vec3& Camera3D::getPosition() const
{
   return _position;
}

void Camera3D::setPosition(const glm::vec3& position)
{
   _position = position;
   updateViewMatrix();  // Update view matrix when Camera3D position changes
}

const glm::vec3& Camera3D::getLookAtPoint() const
{
   return _look_at_point;
}

void Camera3D::setLookAtPoint(const glm::vec3& look_at_point)
{
   _look_at_point = look_at_point;
   updateViewMatrix();  // Update view matrix when look-at point changes
}

void Camera3D::updateViewMatrix()
{
   _view_matrix = glm::lookAt(
      _position,                   // Camera3D position
      _look_at_point,              // Look at point
      glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
   );
}

glm::mat4 Camera3D::getViewMatrixCopy() const
{
   return _view_matrix;
}

const glm::mat4& Camera3D::getViewMatrix() const
{
   return _view_matrix;
}

void Camera3D::setViewMatrix(const glm::mat4& view_matrix)
{
   _view_matrix = view_matrix;
}

float Camera3D::getFOV() const
{
   return _fov;
}

void Camera3D::setFOV(float fov)
{
   _fov = fov;
   updateProjectionMatrix();
}

const std::array<int32_t, 2>& Camera3D::getScreenDimensions() const
{
   return _screen_dimensions;
}
