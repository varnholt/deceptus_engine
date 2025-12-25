#pragma once

#include <memory>
#include "object3d.h"
#include "vbos/vbosphere.h"

class SphereObject : public Object3D
{
public:
   SphereObject(float radius = 1.0f, int slices = 50, int stacks = 50);
   virtual ~SphereObject() = default;

   void update(float deltaTime) override;
   void render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) override;

   void setRotationSpeed(float speed)
   {
      _rotationSpeed = speed;
   }

   float getRotationSpeed() const
   {
      return _rotationSpeed;
   }

private:
   std::unique_ptr<VBOSphere> _sphere;
   float _rotationSpeed{0.5f};
   float _currentRotation{0.0f};
};
