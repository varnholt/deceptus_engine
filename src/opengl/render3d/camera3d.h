#pragma once

#include <array>
#include "opengl/glm/glm.hpp"

/// \brief perspective camera storing projection and view configuration.
class Camera3D
{
public:
   Camera3D() = default;

   /// \brief initializes camera dimensions using current near/far plane values.
   /// \param w viewport width in pixels.
   /// \param h viewport height in pixels.
   void initialize(int32_t w, int32_t h);

   /// \brief initializes camera dimensions and clipping planes.
   /// \param w viewport width in pixels.
   /// \param h viewport height in pixels.
   /// \param nearPlane near clipping distance.
   /// \param farPlane far clipping distance.
   void initialize(int32_t w, int32_t h, float nearPlane, float farPlane);

   /// \brief returns the current field of view in degrees.
   /// \return vertical field of view angle.
   float getFOV() const;

   /// \brief updates field of view and rebuilds projection matrix.
   /// \param fov vertical field of view angle in degrees.
   void setFOV(float fov);

   /// \brief returns the current projection matrix.
   /// \return constant projection matrix reference.
   const glm::mat4& getProjectionMatrix() const;

   /// \brief replaces the projection matrix without changing stored camera params.
   /// \param projection_matrix projection transform to store.
   void setProjectionMatrix(const glm::mat4& projection_matrix);

   /// \brief returns a copy of the current view matrix.
   /// \return view matrix value copy.
   glm::mat4 getViewMatrixCopy() const;

   /// \brief returns the current view matrix by reference.
   /// \return constant view matrix reference.
   const glm::mat4& getViewMatrix() const;

   /// \brief replaces the view matrix directly.
   /// \param view_matrix view transform to store.
   void setViewMatrix(const glm::mat4& view_matrix);

   /// \brief returns current camera world position.
   /// \return constant position vector reference.
   const glm::vec3& getPosition() const;

   /// \brief sets camera world position and recomputes view matrix.
   /// \param Camera3D_position new camera position.
   void setPosition(const glm::vec3& Camera3D_position);

   /// \brief returns current look-at target point.
   /// \return constant target vector reference.
   const glm::vec3& getLookAtPoint() const;

   /// \brief sets look-at target and recomputes view matrix.
   /// \param look_at_point new target point.
   void setLookAtPoint(const glm::vec3& look_at_point);

   /// \brief rebuilds view matrix from position, look-at point and world up.
   void updateViewMatrix();

   /// \brief returns the configured viewport dimensions.
   /// \return width and height stored during initialization.
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
