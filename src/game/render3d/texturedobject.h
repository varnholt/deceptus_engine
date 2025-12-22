#pragma once

#include <memory>
#include "game/render3d/3dobject.h"
#include "opengl/vbos/vbomesh.h"

namespace deceptus {
namespace render3d {

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
   );  // Default to true to maintain existing behavior
   virtual ~TexturedObject();

   void update(float deltaTime) override;
   void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

   void setRotationSpeed(const glm::vec3& speed)
   {
      _rotationSpeed = speed;
   }
   glm::vec3 getRotationSpeed() const
   {
      return _rotationSpeed;
   }
   void setUseLighting(bool use)
   {
      _useLighting = use;
   }
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

} // namespace render3d
} // namespace deceptus