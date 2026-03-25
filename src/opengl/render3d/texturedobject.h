#pragma once

#include <memory>
#include "opengl/render3d/object3d.h"
#include "opengl/vbos/vbomesh.h"

/// \brief textured mesh object with optional lighting and continuous rotation.
class TexturedObject : public Object3D
{
public:
   /// \brief loads a mesh and texture and configures initial render behavior.
   /// \param obj_file_path path to the mesh .obj file.
   /// \param texture_file_path path to the texture image file.
   /// \param scale scale applied while loading mesh vertices.
   /// \param recenter_mesh whether to center mesh geometry around origin.
   /// \param load_texture_coordinates whether texture coordinates are read from obj.
   /// \param use_lighting whether shader lighting contribution is enabled.
   TexturedObject(
      const std::string& obj_file_path,
      const std::string& texture_file_path,
      float scale = 1.0f,
      bool recenter_mesh = true,
      bool load_texture_coordinates = true,
      bool use_lighting = true
   );

   /// \brief releases allocated OpenGL texture resources.
   virtual ~TexturedObject() override;

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
   void setRotationSpeed(const glm::vec3& speed);

   /// \brief returns current angular rotation speed.
   /// \return per-axis angular speed in radians per second.
   glm::vec3 getRotationSpeed() const;

   /// \brief enables or disables lighting contribution during rendering.
   /// \param use true to enable lighting uniforms, false for texture-focused shading.
   void setUseLighting(bool use);

   /// \brief reports whether lighting contribution is enabled.
   /// \return true when lighting is enabled.
   bool getUseLighting() const;

private:
   void loadTexture(const std::string& texture_file_path);

   std::unique_ptr<VBOMesh> _mesh;
   glm::vec3 _rotation_speed{0.0f, 0.5f, 0.0f};
   glm::vec3 _current_rotation{0.0f, 0.0f, 0.0f};
   GLuint _texture_id{0};
   bool _use_lighting{true};
};
