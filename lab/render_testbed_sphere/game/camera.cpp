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

void Camera::initialize(int32_t w, int32_t h, float nearPlane, float farPlane)
{
   _projection_matrix = glm::perspective(
      glm::radians(70.0f),
      static_cast<float>(w) / static_cast<float>(h),
      nearPlane,
      farPlane
   );

   _screen_dimensions[0] = w;
   _screen_dimensions[1] = h;

   // Initialize the view matrix with default values
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




const std::array<int32_t, 2>& Camera::getScreenDimensions() const
{
   return _screen_dimensions;
}


