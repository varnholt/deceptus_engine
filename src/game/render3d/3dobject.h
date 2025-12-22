#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "opengl/glslprogram.h"
#include <memory>

namespace deceptus {
namespace render3d {

class Object3D
{
public:
    Object3D() = default;
    virtual ~Object3D() = default;

    virtual void update(float deltaTime) = 0;
    virtual void render(const std::shared_ptr<GLSLProgram>& shader,
                       const glm::mat4& view_matrix,
                       const glm::mat4& projection_matrix) = 0;
    virtual void setPosition(const glm::vec3& position) { _position = position; }
    virtual glm::vec3 getPosition() const { return _position; }

    virtual void setRotation(const glm::vec3& rotation) { _rotation = rotation; }
    virtual glm::vec3 getRotation() const { return _rotation; }

    virtual void setScale(const glm::vec3& scale) { _scale = scale; }
    virtual glm::vec3 getScale() const { return _scale; }

protected:
    glm::vec3 _position{0.0f, 0.0f, 0.0f};
    glm::vec3 _rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
};

} // namespace render3d
} // namespace deceptus