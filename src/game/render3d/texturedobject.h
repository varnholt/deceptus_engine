#pragma once

#include <memory>
#include "game/render3d/object3d.h"
#include "opengl/vbos/vbomesh.h"

class TexturedObject : public Object3D
{
public:
   TexturedObject(
      const std::string& objFile,
      const std::string& textureFile,
      float scale = 1.0f,
      bool reCenterMesh = true,
      bool loadTc = true,
      bool useLighting = true
   );

   virtual ~TexturedObject();

   void update(float deltaTime) override;
   void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

   void setRotationSpeed(const glm::vec3& speed);
   glm::vec3 getRotationSpeed() const;

   void setUseLighting(bool use);
   bool getUseLighting() const;

private:
   void loadTexture(const std::string& textureFile);

   std::unique_ptr<VBOMesh> _mesh;
   glm::vec3 _rotationSpeed{0.0f, 0.5f, 0.0f};
   glm::vec3 _currentRotation{0.0f, 0.0f, 0.0f};
   GLuint _textureId{0};
   bool _useLighting{true};
};
