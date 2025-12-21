#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "opengl/glslprogram.h"
#include <memory>

namespace deceptus {
namespace menu3d {

class Menu3DObject
{
public:
    Menu3DObject() = default;
    virtual ~Menu3DObject() = default;

    virtual void update(float deltaTime) = 0;
    virtual void
    render(const std::shared_ptr<class GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix) = 0;

    void setPosition(const glm::vec3& pos)
    {
        _position = pos;
    }
    glm::vec3 getPosition() const
    {
        return _position;
    }
    void setScale(const glm::vec3& scale)
    {
        _scale = scale;
    }
    glm::vec3 getScale() const
    {
        return _scale;
    }
    void setRotation(const glm::vec3& rot)
    {
        _rotation = rot;
    }
    glm::vec3 getRotation() const
    {
        return _rotation;
    }

protected:
    glm::vec3 _position{0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
    glm::vec3 _rotation{0.0f, 0.0f, 0.0f};
};

} // namespace menu3d
} // namespace deceptus