#include "menu3drenderer.h"

#include "game/shaders/shaderpool.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace deceptus {
namespace menu3d {

// Menu3DCamera implementation
Menu3DCamera::Menu3DCamera()
{
    _viewMatrix = glm::mat4(1.0f);
    _projectionMatrix = glm::mat4(1.0f);
}

void Menu3DCamera::initialize(int width, int height, float near_plane, float far_plane)
{
    _width = width;
    _height = height;
    _near = near_plane;
    _far = far_plane;
    _projDirty = true;
    _viewDirty = true;
}

void Menu3DCamera::update(float deltaTime)
{
    // Update camera if needed
    if (_viewDirty) {
        updateViewMatrix();
    }
    if (_projDirty) {
        updateProjectionMatrix();
    }
}

void Menu3DCamera::updateViewMatrix()
{
    _viewMatrix = glm::lookAt(_cameraPosition, _lookAtPoint, glm::vec3(0.0f, 1.0f, 0.0f));
    _viewDirty = false;
}

void Menu3DCamera::updateProjectionMatrix()
{
    float aspect = static_cast<float>(_width) / static_cast<float>(_height);
    _projectionMatrix = glm::perspective(glm::radians(_fov), aspect, _near, _far);
    _projDirty = false;
}

void Menu3DCamera::setCameraPosition(const glm::vec3& pos)
{
    _cameraPosition = pos;
    _viewDirty = true;
}

void Menu3DCamera::setLookAtPoint(const glm::vec3& target)
{
    _lookAtPoint = target;
    _viewDirty = true;
}


// Menu3DRenderer implementation
Menu3DRenderer::Menu3DRenderer()
{
}

Menu3DRenderer::~Menu3DRenderer()
{
    _objects.clear();
}

void Menu3DRenderer::initialize()
{
    if (_initialized) {
        return;
    }

    // Initialize the camera
    _camera = std::make_unique<Menu3DCamera>();
    _camera->initialize(800, 600, 0.1f, 100.0f); // Default size, will be updated before rendering

    // Initialize required shaders
    auto& shader_pool = ShaderPool::getInstance();
    shader_pool.add("menu3d", "data/shaders/menu3d.vs", "data/shaders/menu3d.fs");

    _shader = shader_pool.get("menu3d");
    if (!_shader) {
        std::cerr << "Failed to load menu3d shader!" << std::endl;
        return;
    }

    _initialized = true;
}

void Menu3DRenderer::update(const sf::Time& deltaTime)
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

void Menu3DRenderer::render(sf::RenderTarget& target)
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
    _shader->setUniform("useSpecular", true);
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

void Menu3DRenderer::add3DObject(std::shared_ptr<Menu3DObject> object)
{
    _objects.push_back(object);
}

void Menu3DRenderer::setupOpenGLState()
{
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set clear color and clear buffers
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Dark blueish background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Menu3DRenderer::restoreOpenGLState()
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

void Menu3DRenderer::clear3DObjects()
{
    _objects.clear();
}

} // namespace menu3d
} // namespace deceptus