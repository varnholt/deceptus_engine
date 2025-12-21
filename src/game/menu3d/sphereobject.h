#pragma once

#include "opengl/vbos/vbosphere.h"
#include "opengl/glslprogram.h"
#include "menu3dobject.h"  // Include the Menu3DObject base class
#include <memory>

namespace deceptus {
namespace menu3d {

class SphereObject : public deceptus::menu3d::Menu3DObject
{
public:
    SphereObject(float radius = 1.0f, int slices = 50, int stacks = 50);
    virtual ~SphereObject() = default;

    void update(float deltaTime) override;
    void render(const std::shared_ptr<GLSLProgram>& shader,
                const glm::mat4& view_matrix,
                const glm::mat4& projection_matrix) override;

    void setRotationSpeed(float speed) { _rotationSpeed = speed; }
    float getRotationSpeed() const { return _rotationSpeed; }

private:
    std::unique_ptr<VBOSphere> _sphere;
    float _rotationSpeed{0.5f};
    float _currentRotation{0.0f};
};

} // namespace menu3d
} // namespace deceptus