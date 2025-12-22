#include "renderer3d.h"

#include "game/shaders/shaderpool.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace deceptus {
namespace render3d {

// Camera3D implementation
Camera3D::Camera3D()
{
    _viewMatrix = glm::mat4(1.0f);
    _projectionMatrix = glm::mat4(1.0f);
}

void Camera3D::initialize(int width, int height, float near_plane, float far_plane)
{
    _width = width;
    _height = height;
    _near = near_plane;
    _far = far_plane;
    _projDirty = true;
    _viewDirty = true;
}

void Camera3D::update(float deltaTime)
{
    // Update camera if needed
    if (_viewDirty) {
        updateViewMatrix();
    }
    if (_projDirty) {
        updateProjectionMatrix();
    }
}

void Camera3D::updateViewMatrix()
{
    _viewMatrix = glm::lookAt(_cameraPosition, _lookAtPoint, glm::vec3(0.0f, 1.0f, 0.0f));
    _viewDirty = false;
}

void Camera3D::updateProjectionMatrix()
{
    float aspect = static_cast<float>(_width) / static_cast<float>(_height);
    _projectionMatrix = glm::perspective(glm::radians(_fov), aspect, _near, _far);
    _projDirty = false;
}

void Camera3D::setCameraPosition(const glm::vec3& pos)
{
    _cameraPosition = pos;
    _viewDirty = true;
}

void Camera3D::setLookAtPoint(const glm::vec3& target)
{
    _lookAtPoint = target;
    _viewDirty = true;
}


// Renderer3D implementation
Renderer3D::Renderer3D()
{
}

Renderer3D::~Renderer3D()
{
    _objects.clear();
}

void Renderer3D::initialize()
{
    if (_initialized) {
        return;
    }

    // Initialize the camera
    _camera = std::make_unique<Camera3D>();
    _camera->initialize(800, 600, 0.1f, 100.0f); // Default size, will be updated before rendering

    // Initialize required shaders
    auto& shader_pool = ShaderPool::getInstance();
    shader_pool.add("render3d", "data/shaders/render3d/render3d.vs", "data/shaders/render3d/render3d.fs");

    _shader = shader_pool.get("render3d");
    if (!_shader) {
        std::cerr << "Failed to load render3d shader!" << std::endl;
        return;
    }

    _initialized = true;
}

void Renderer3D::update(const sf::Time& deltaTime)
{
    if (!_initialized) {
        return;
    }

    float dt = deltaTime.asSeconds();

    // Update all 3D objects
    for (auto& obj : _objects) {
        obj->update(dt);
    }
}

void Renderer3D::render(sf::RenderTarget& target)
{
    if (!_initialized || _objects.empty() || !_shader) {
        return;
    }

    // Get current target size and set viewport
    sf::Vector2u target_size = target.getSize();
    _camera->initialize(static_cast<int>(target_size.x), static_cast<int>(target_size.y));

    // Setup OpenGL state for 3D rendering
    setupOpenGLState();

    // Use the shader
    _shader->use();

    // Set shader uniforms
    _shader->setUniform("Light.Position", glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));
    _shader->setUniform("Light.Intensity", glm::vec3(1.0f, 1.0f, 1.0f));
    _shader->setUniform("useAO", false);
    _shader->setUniform("useSpecular", false);  // Disable specular to match original starmap appearance
    _shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

    // Get matrices from camera
    glm::mat4 view_matrix = _camera->getViewMatrix();
    glm::mat4 projection_matrix = _camera->getProjectionMatrix();

    // Render all 3D objects
    for (auto& obj : _objects) {
        obj->render(_shader, view_matrix, projection_matrix);
    }

    // Restore OpenGL state after 3D rendering
    restoreOpenGLState();
}

void Renderer3D::add3DObject(std::shared_ptr<Object3D> object)
{
    _objects.push_back(object);
}

void Renderer3D::setupOpenGLState()
{
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set clear color and clear buffers - using red to verify 3D rendering is happening
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    // glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red background to verify 3D rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer3D::restoreOpenGLState()
{
    // Disable depth test to return to 2D rendering state
    glDisable(GL_DEPTH_TEST);

    // Enable blending for 2D rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Reset OpenGL bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void Renderer3D::clear3DObjects()
{
    _objects.clear();
}

} // namespace render3d
} // namespace deceptus