#pragma once

#include "object3d.h"
#include "vbos/vbomesh.h"
#include <memory>

class TexturedObject : public Object3D
{
public:
    TexturedObject(const std::string& objFile, 
                   const std::string& textureFile,
                   float scale = 1.0f,
                   bool reCenterMesh = true,
                   bool loadTc = true);
    virtual ~TexturedObject();

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
    GLuint _textureId{0};
    
    void loadTexture(const std::string& textureFile);
};