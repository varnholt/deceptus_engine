#pragma once

#include <memory>
#include "game/render3d/3dobject.h"  // Include the base 3D object class
#include "opengl/glslprogram.h"
#include "opengl/vbos/vbomesh.h"

/// \brief textured sphere-like object that rotates and renders with a mesh texture.
class TexturedSphereObject : public Object3D
{
public:
   /// \brief loads a mesh and texture for the textured sphere object.
   /// \param objFile path to the mesh .obj file.
   /// \param textureFile path to the texture image file.
   /// \param scale scale applied while loading mesh vertices.
   /// \param reCenterMesh whether to center mesh geometry around origin.
   /// \param loadTc whether to load texture coordinates from mesh file.
   TexturedSphereObject(
      const std::string& objFile,
      const std::string& textureFile,
      float scale = 1.0f,
      bool reCenterMesh = true,
      bool loadTc = true
   );

   /// \brief releases allocated OpenGL texture resources.
   virtual ~TexturedSphereObject();

   /// \brief updates internal rotation accumulator based on rotation speed.
   /// \param deltaTime elapsed time in seconds.
   void update(float deltaTime) override;

   /// \brief renders the textured mesh and uploads transform/material uniforms.
   /// \param shader shader used for drawing and uniform updates.
   /// \param view_matrix current camera view matrix.
   /// \param projection_matrix current camera projection matrix.
   void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

   /// \brief sets angular rotation speed applied during update.
   /// \param speed per-axis angular speed in radians per second.
   void setRotationSpeed(const glm::vec3& speed)
   {
      _rotationSpeed = speed;
   }

   /// \brief returns current angular rotation speed.
   /// \return per-axis angular speed in radians per second.
   glm::vec3 getRotationSpeed() const
   {
      return _rotationSpeed;
   }

   /// \brief enables or disables lighting state tracked by this object.
   /// \param use true to enable lighting flag, false to disable it.
   void setUseLighting(bool use)
   {
      _useLighting = use;
   }

   /// \brief returns whether lighting is marked as enabled.
   /// \return true when lighting flag is enabled.
   bool getUseLighting() const
   {
      return _useLighting;
   }

private:
   std::unique_ptr<VBOMesh> _mesh;
   glm::vec3 _rotationSpeed{0.0f, 0.5f, 0.0f};  // Default rotation around Y-axis
   glm::vec3 _currentRotation{0.0f, 0.0f, 0.0f};
   GLuint _textureId{0};
   bool _useLighting{true};  // Default to true to maintain existing behavior

   void loadTexture(const std::string& textureFile);
};
