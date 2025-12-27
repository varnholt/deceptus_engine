#pragma once

#include <memory>
#include "opengl/render3d/object3d.h"
#include "opengl/vbos/vbomesh.h"

class TexturedObject : public Object3D
{
public:
   TexturedObject(
      const std::string& obj_file_path,
      const std::string& texture_file_path,
      float scale = 1.0f,
      bool recenter_mesh = true,
      bool load_texture_coordinates = true,
      bool use_lighting = true
   );

   virtual ~TexturedObject() override;

   void update(float deltaTime) override;
   void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

   void setRotationSpeed(const glm::vec3& speed);
   glm::vec3 getRotationSpeed() const;

   void setUseLighting(bool use);
   bool getUseLighting() const;

private:
   void loadTexture(const std::string& texture_file_path);

   std::unique_ptr<VBOMesh> _mesh;
   glm::vec3 _rotation_speed{0.0f, 0.5f, 0.0f};
   glm::vec3 _current_rotation{0.0f, 0.0f, 0.0f};
   GLuint _texture_id{0};
   bool _use_lighting{true};
};
