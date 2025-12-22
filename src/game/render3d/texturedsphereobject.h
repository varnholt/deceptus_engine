#pragma once

#include <memory>
#include "opengl/vbos/vbosphere.h"
#include "opengl/glslprogram.h"
#include "game/render3d/3dobject.h"  // Include the base 3D object class

namespace deceptus {
namespace render3d {

class TexturedSphereObject : public deceptus::render3d::Object3D
{
public:
   TexturedSphereObject(
      const std::string& textureFile,
      float radius = 1.0f,
      int slices = 50,
      int stacks = 50
   );
   virtual ~TexturedSphereObject();

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
   std::unique_ptr<VBOSphere> _sphere;
   glm::vec3 _rotationSpeed{0.0f, 0.5f, 0.0f};  // Default rotation around Y-axis
   glm::vec3 _currentRotation{0.0f, 0.0f, 0.0f};
   GLuint _textureId{0};
   bool _useLighting{true};  // Default to true to maintain existing behavior

   void loadTexture(const std::string& textureFile);
};

} // namespace render3d
} // namespace deceptus