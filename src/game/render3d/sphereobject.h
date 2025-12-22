#pragma once

#include "opengl/vbos/vbomesh.h"
#include "opengl/glslprogram.h"
#include "game/render3d/3dobject.h"  // Include the base 3D object class
#include <memory>
#include <string>

namespace deceptus {
namespace render3d {

class SphereObject : public deceptus::render3d::Object3D
{
public:
    SphereObject(const std::string& objFile,
                 float scale = 1.0f,
                 bool reCenterMesh = true,
                 bool loadTc = true);
    virtual ~SphereObject() = default;

    void update(float deltaTime) override;
    void render(const std::shared_ptr<GLSLProgram>& shader,
                const glm::mat4& view_matrix,
                const glm::mat4& projection_matrix) override;

    void setRotationSpeed(float speed) { _rotationSpeed = speed; }
    float getRotationSpeed() const { return _rotationSpeed; }

private:
    std::unique_ptr<VBOMesh> _mesh;
    float _rotationSpeed{0.5f};
    float _currentRotation{0.0f};
};

} // namespace render3d
} // namespace deceptus