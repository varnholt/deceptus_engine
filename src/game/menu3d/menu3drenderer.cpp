#include "menu3drenderer.h"

#include "game/shaders/shaderpool.h"
#include "opengl/vbos/vbosphere.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace deceptus {
namespace menu3d {

// StarmapObject implementation
StarmapObject::StarmapObject(
   const std::string& objFile,
   const std::string& textureFile,
   float scale,
   bool reCenterMesh,
   bool loadTc,
   bool useLighting
)
    : _mesh(std::make_unique<VBOMesh>(objFile.c_str(), scale, reCenterMesh, loadTc)), _useLighting(useLighting)
{
   loadTexture(textureFile);
}

StarmapObject::~StarmapObject()
{
   if (_textureId != 0)
   {
      glDeleteTextures(1, &_textureId);
      _textureId = 0;
   }
}

void StarmapObject::update(float deltaTime)
{
   _currentRotation += _rotationSpeed * deltaTime;
}

void StarmapObject::render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
{
   if (!_mesh || !shader || _textureId == 0)
   {
      return;
   }

   // Create model matrix with position, rotation, and scale
   glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), _position);

   // Apply rotations around X, Y, and Z axes
   model_matrix = glm::rotate(model_matrix, _currentRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));  // X-axis
   model_matrix = glm::rotate(model_matrix, _currentRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));  // Y-axis
   model_matrix = glm::rotate(model_matrix, _currentRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));  // Z-axis

   model_matrix = glm::scale(model_matrix, _scale);

   // Calculate MVP matrices
   glm::mat4 mv_matrix = view_matrix * model_matrix;
   glm::mat4 mvp_matrix = projection_matrix * mv_matrix;
   glm::mat3 normal_matrix = glm::mat3(
      glm::vec3(mv_matrix[0]),
      glm::vec3(mv_matrix[1]),
      glm::vec3(mv_matrix[2])
   );

   // Set the uniforms for the textured mesh
   shader->setUniform("MVP", mvp_matrix);
   shader->setUniform("ModelViewMatrix", mv_matrix);
   shader->setUniform("ModelMatrix", model_matrix);
   shader->setUniform("NormalMatrix", normal_matrix);

   // Set material properties for textured mesh
   shader->setUniform("Material.Kd", glm::vec3(1.0f, 1.0f, 1.0f));  // Use texture for diffuse
   shader->setUniform("Material.Ks", glm::vec3(0.5f, 0.5f, 0.5f));
   shader->setUniform("Material.Ka", glm::vec3(0.3f, 0.3f, 0.3f));
   shader->setUniform("Material.Shininess", 32.0f);

   // Set the lighting uniform
   shader->setUniform("useLighting", _useLighting);

   // Bind the texture to texture unit 0
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _textureId);

   // Tell the shader which texture unit we're using
   shader->setUniform("Tex1", 0);

   _mesh->render();
}

void StarmapObject::loadTexture(const std::string& textureFile)
{
   if (_textureId != 0)
   {
      glDeleteTextures(1, &_textureId);
      _textureId = 0;
   }

   GLint w, h;
   GLubyte* textureData = TGAIO::read(textureFile.c_str(), w, h);
   if (textureData)
   {
      glGenTextures(1, &_textureId);
      glBindTexture(GL_TEXTURE_2D, _textureId);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);

      delete[] textureData;
   }
   else
   {
      std::cerr << "Failed to load texture: " << textureFile << std::endl;
   }
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