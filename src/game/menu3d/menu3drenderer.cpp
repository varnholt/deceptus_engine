#include "menu3drenderer.h"

#include "game/shaders/shaderpool.h"
#include "opengl/vbos/vbosphere.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace deceptus {
namespace menu3d {

// SkyboxObject implementation
SkyboxObject::SkyboxObject(float radius, int slices, int stacks)
    : _sphere(std::make_unique<VBOSphere>(radius, slices, stacks))
{
    // Set default scale to make it large enough to be a skybox
    setScale(glm::vec3(10.0f, 10.0f, 10.0f));
}

void SkyboxObject::update(float deltaTime)
{
    _currentRotation += _rotationSpeed * deltaTime;
}

void SkyboxObject::render(const std::shared_ptr<GLSLProgram>& shader,
                         const glm::mat4& view_matrix,
                         const glm::mat4& projection_matrix)
{
    if (!_sphere || !shader) {
        return;
    }

    // Create model matrix with position, rotation, and scale
    glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), _position);
    model_matrix = glm::rotate(model_matrix, _currentRotation.y + _rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, _currentRotation.x + _rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix, _currentRotation.z + _rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = glm::scale(model_matrix, _scale);

    // Calculate MVP matrices
    glm::mat4 mv_matrix = view_matrix * model_matrix;
    glm::mat4 mvp_matrix = projection_matrix * mv_matrix;
    glm::mat3 normal_matrix = glm::mat3(
        glm::vec3(mv_matrix[0]),
        glm::vec3(mv_matrix[1]),
        glm::vec3(mv_matrix[2])
    );

    // Set the uniforms for the skybox
    shader->setUniform("MVP", mvp_matrix);
    shader->setUniform("ModelViewMatrix", mv_matrix);
    shader->setUniform("ModelMatrix", model_matrix);
    shader->setUniform("NormalMatrix", normal_matrix);

    // Set material properties for the skybox
    shader->setUniform("Material.Kd", 0.8f, 0.9f, 1.0f);  // Light blueish diffuse
    shader->setUniform("Material.Ks", 0.2f, 0.2f, 0.2f);
    shader->setUniform("Material.Ka", 0.5f, 0.6f, 0.8f);
    shader->setUniform("Material.Shininess", 10.0f);

    // Set additional uniforms for special skybox effects
    shader->setUniform("useLighting", true);
    shader->setUniform("DrawSkyBox", true);

    _sphere->render();
}

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

    _camera = std::make_unique<Menu3DCamera>();
    _camera->initialize(800, 600); // Default size, will be updated before rendering

    // Initialize the shader for 3D rendering
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
    
    // Update camera
    _camera->update(dt);

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
    glm::mat4 view_matrix = _camera->getViewMatrixCopy();
    glm::mat4 projection_matrix = _camera->getProjectionMatrix();

    // Render all 3D objects
    for (auto& obj : _objects) {
        obj->render(_shader, view_matrix, projection_matrix);
    }

    restoreOpenGLState();
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

void Menu3DRenderer::add3DObject(std::shared_ptr<Menu3DObject> object)
{
    _objects.push_back(object);
}

void Menu3DRenderer::clear3DObjects()
{
    _objects.clear();
}

} // namespace menu3d
} // namespace deceptus