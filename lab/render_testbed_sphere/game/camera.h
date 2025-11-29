#pragma once

#include <array>
#include "../glm/glm.hpp"

class Camera
{
   public:
      static Camera& getInstance();
      void initialize(int32_t w, int32_t h);
      void initialize(int32_t w, int32_t h, float nearPlane, float farPlane);

      const glm::mat4& getProjectionMatrix() const;
      void setProjectionMatrix(const glm::mat4& projection_matrix);

      glm::mat4 getViewMatrixCopy() const;
      const glm::mat4& getViewMatrix() const;
      void setViewMatrix(const glm::mat4& view_matrix);

      // Camera position and look-at point controls
      const glm::vec3& getCameraPosition() const;
      void setCameraPosition(const glm::vec3& camera_position);

      const glm::vec3& getLookAtPoint() const;
      void setLookAtPoint(const glm::vec3& look_at_point);

      // Recalculate view matrix based on current camera position and look-at point
      void updateViewMatrix();

      const std::array<int32_t, 2>& getScreenDimensions() const;


   private:
      Camera() = default;

      glm::mat4 _projection_matrix;
      glm::mat4 _view_matrix;
      glm::vec3 _camera_position;
      glm::vec3 _look_at_point{0.0f, 0.0f, 0.0f};  // Default look-at point at origin

      std::array<int32_t, 2> _screen_dimensions;
};

