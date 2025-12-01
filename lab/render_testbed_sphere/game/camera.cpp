#include "camera.h"

// glm
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtx/transform.hpp"



Camera& Camera::getInstance()
{
   static Camera __camera;
   return __camera;
}


void Camera::initialize(int32_t w, int32_t h)
{
   initialize(w, h, 0.3f, 1000.0f);  // Use default values
}

void Camera::updateProjectionMatrix()
{
   _projection_matrix = glm::perspective(
      glm::radians(_fov), static_cast<float>(_screen_dimensions[0]) / static_cast<float>(_screen_dimensions[1]), _near_plane, _far_plane
   );
}

void Camera::initialize(int32_t w, int32_t h, float nearPlane, float farPlane)
{
   _near_plane = nearPlane;
   _far_plane = farPlane;
   _screen_dimensions[0] = w;
   _screen_dimensions[1] = h;

   updateProjectionMatrix();
   updateViewMatrix();
}


const glm::mat4& Camera::getProjectionMatrix() const
{
   return _projection_matrix;
}


void Camera::setProjectionMatrix(const glm::mat4& projection_matrix)
{
   _projection_matrix = projection_matrix;
}


const glm::vec3& Camera::getCameraPosition() const
{
   return _camera_position;
}


void Camera::setCameraPosition(const glm::vec3& camera_position)
{
   _camera_position = camera_position;
   updateViewMatrix();  // Update view matrix when camera position changes
}


const glm::vec3& Camera::getLookAtPoint() const
{
   return _look_at_point;
}


void Camera::setLookAtPoint(const glm::vec3& look_at_point)
{
   _look_at_point = look_at_point;
   updateViewMatrix();  // Update view matrix when look-at point changes
}


void Camera::updateViewMatrix()
{
   _view_matrix = glm::lookAt(
      _camera_position,  // Camera position
      _look_at_point,    // Look at point
      glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
   );
}


glm::mat4 Camera::getViewMatrixCopy() const
{
   return _view_matrix;
}


const glm::mat4& Camera::getViewMatrix() const
{
   return _view_matrix;
}


void Camera::setViewMatrix(const glm::mat4& view_matrix)
{
   _view_matrix = view_matrix;
}


float Camera::getFOV() const
{
   return _fov;
}


void Camera::setFOV(float fov)
{
   _fov = fov;
   updateProjectionMatrix();
}

const std::array<int32_t, 2>& Camera::getScreenDimensions() const
{
   return _screen_dimensions;
}


