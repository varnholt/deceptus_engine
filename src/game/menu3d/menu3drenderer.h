#pragma once

#include <memory>
#include <vector>
#include "opengl/gl_current.h"  // Include GLEW first
#include "opengl/glslprogram.h"
#include "opengl/vbos/vbosphere.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace deceptus {
namespace menu3d {

class Menu3DObject
{
public:
    Menu3DObject() = default;
    virtual ~Menu3DObject() = default;

    virtual void update(float deltaTime) = 0;
    virtual void render(const std::shared_ptr<class GLSLProgram>& shader,
                       const glm::mat4& view_matrix,
                       const glm::mat4& projection_matrix) = 0;

    void setPosition(const glm::vec3& pos) { _position = pos; }
    glm::vec3 getPosition() const { return _position; }
    void setScale(const glm::vec3& scale) { _scale = scale; }
    glm::vec3 getScale() const { return _scale; }
    void setRotation(const glm::vec3& rot) { _rotation = rot; }
    glm::vec3 getRotation() const { return _rotation; }

protected:
    glm::vec3 _position{0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
    glm::vec3 _rotation{0.0f, 0.0f, 0.0f};
};

class SkyboxObject : public Menu3DObject
{
public:
    SkyboxObject(float radius = 1.0f, int slices = 50, int stacks = 50);
    virtual ~SkyboxObject() = default;

    void update(float deltaTime) override;
    void render(const std::shared_ptr<class GLSLProgram>& shader,
                const glm::mat4& view_matrix,
                const glm::mat4& projection_matrix) override;

    void setRotationSpeed(const glm::vec3& speed) { _rotationSpeed = speed; }
    glm::vec3 getRotationSpeed() const { return _rotationSpeed; }

private:
    std::unique_ptr<class VBOSphere> _sphere;
    glm::vec3 _rotationSpeed{0.0f, 0.01f, 0.0f};
    glm::vec3 _currentRotation{0.0f, 0.0f, 0.0f};
};

class Menu3DCamera
{
public:
    Menu3DCamera();
    virtual ~Menu3DCamera() = default;

    void initialize(int width, int height, float near_plane = 0.1f, float far_plane = 100.0f);
    void update(float deltaTime);

    const glm::mat4& getViewMatrix() const { return _viewMatrix; }
    const glm::mat4& getProjectionMatrix() const { return _projectionMatrix; }
    glm::mat4 getViewMatrixCopy() const { return _viewMatrix; }
    
    void setCameraPosition(const glm::vec3& pos) { _cameraPosition = pos; _viewDirty = true; }
    glm::vec3 getCameraPosition() const { return _cameraPosition; }
    void setLookAtPoint(const glm::vec3& target) { _lookAtPoint = target; _viewDirty = true; }
    glm::vec3 getLookAtPoint() const { return _lookAtPoint; }
    void setFOV(float fov) { _fov = fov; _projDirty = true; }
    float getFOV() const { return _fov; }

private:
    void updateViewMatrix();
    void updateProjectionMatrix();

    glm::vec3 _cameraPosition{0.0f, 0.0f, 5.0f};
    glm::vec3 _lookAtPoint{0.0f, 0.0f, 0.0f};
    float _fov{70.0f};
    float _near{0.1f};
    float _far{100.0f};
    int _width{800};
    int _height{600};

    glm::mat4 _viewMatrix{1.0f};
    glm::mat4 _projectionMatrix{1.0f};

    bool _viewDirty{true};
    bool _projDirty{true};
};

class Menu3DRenderer
{
public:
    Menu3DRenderer();
    ~Menu3DRenderer();

    void initialize();
    void update(const sf::Time& deltaTime);
    void render(sf::RenderTarget& target);

    void add3DObject(std::shared_ptr<Menu3DObject> object);
    void clear3DObjects();

private:
    void setupOpenGLState();
    void restoreOpenGLState();

    std::unique_ptr<Menu3DCamera> _camera;
    std::vector<std::shared_ptr<Menu3DObject>> _objects;
    std::shared_ptr<class GLSLProgram> _shader;
    bool _initialized{false};
};

} // namespace menu3d
} // namespace deceptus