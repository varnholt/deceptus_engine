#include "testmechanism.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/OpenGL.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/gl_current.h"

// Include the shader system from shmup
#include "game/shaderpool.h"
// Include the image loading system
#include "image/tgaio.h"

TestMechanism::TestMechanism()
{
   load();
}

void TestMechanism::load()
{
   // Initialize the camera with perspective projection (aspect ratio handled properly)
   _camera = &Camera::getInstance();
   _camera->initialize(1280, 720);  // Initialize with window size - aspect ratio will be handled

   // Set initial camera position and look at origin
   _camera->setCameraPosition(glm::vec3(0.0f, 0.0f, 5.0f));

   glm::mat4 view_matrix = glm::lookAt(
      glm::vec3(0.0f, 0.0f, 5.0f),  // Camera position
      glm::vec3(0.0f, 0.0f, 0.0f),  // Look at point
      glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
   );
   _camera->setViewMatrix(view_matrix);

   // Create a sphere VBO - radius 1.0, 50 slices, 50 stacks for smooth appearance
   _sphere = std::make_unique<VBOSphere>(1.0f, 50, 50);

   // Create the starmap mesh VBO from the OBJ file
   _starmapMesh = std::make_unique<VBOMesh>("data/objects/starmap.obj", 1.0f, true, true);

   // Load the starmap texture
   GLint w, h;
   GLubyte* textureData = nullptr;

   textureData = TGAIO::read("data/textures/starmap_color.tga", w, h);
   if (textureData) {
      glGenTextures(1, &_starmapTextureId);
      glBindTexture(GL_TEXTURE_2D, _starmapTextureId);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);

      // Clean up the loaded data
      delete[] textureData;
   }

   // Initialize the required shaders if not already done
   auto& shaderPool = ShaderPool::getInstance();

   // Check if the simple shader already exists, if not, add it
   if (!shaderPool.get("simple"))
   {
      shaderPool.add("simple", "data/shaders/simple.vs", "data/shaders/simple.fs");
   }

   // Check if the texture shader already exists, if not, add it
   if (!shaderPool.get("texture"))
   {
      shaderPool.add("texture", "data/shaders/texture.vs", "data/shaders/texture.fs");
   }

   _initialized = true;
}

void TestMechanism::drawEditor()
{
   ImGui::Begin("3D Sphere Settings");

   ImGui::Text("Rotation Control");
   ImGui::SliderFloat("Rotation Speed", &_rotationSpeed, 0.0f, 5.0f, "%.2f rad/s");

   ImGui::End();
}

void TestMechanism::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   // First, draw the ImGui editor
   // drawEditor();

   if (!_initialized)
   {
      return;
   }

   // Enable depth testing for 3D rendering
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);

   // Set the clear color and clear both color and depth buffers
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Get the texture shader from the shader pool
   const auto& shader = ShaderPool::getInstance().get("texture");
   if (!shader)
   {
      // If shader doesn't exist, return early
      return;
   }

   // Use the shader
   shader->use();

   // Apply rotation to model matrix
   glm::mat4 model_matrix = glm::rotate(
      glm::mat4(1.0f),             // Identity matrix
      _currentRotation,            // Rotation angle
      glm::vec3(0.0f, 1.0f, 0.0f)  // Rotation axis (Y-axis)
   );

   // Get view and projection matrices from camera
   glm::mat4 view_matrix = _camera->getViewMatrixCopy();
   glm::mat4 projection_matrix = _camera->getProjectionMatrix();

   // Calculate MVP matrix - this is what ensures correct aspect ratio rendering
   glm::mat4 mvp_matrix = projection_matrix * view_matrix * model_matrix;

   // Calculate model-view matrix
   glm::mat4 mv_matrix = view_matrix * model_matrix;

   // Calculate normal matrix (upper 3x3 of model-view matrix)
   glm::mat3 normal_matrix = glm::mat3(glm::vec3(mv_matrix[0]), glm::vec3(mv_matrix[1]), glm::vec3(mv_matrix[2]));

   // Set the required uniforms for the texture shader
   shader->setUniform("MVP", mvp_matrix);
   shader->setUniform("ModelViewMatrix", mv_matrix);
   shader->setUniform("ModelMatrix", model_matrix);
   shader->setUniform("NormalMatrix", normal_matrix);
   shader->setUniform("ProjectionMatrix", projection_matrix);

   // Set lighting uniforms (use similar values as in the renderer)
   shader->setUniform("Light.Position", glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));
   shader->setUniform("Light.Intensity", glm::vec3(1.0f, 1.0f, 1.0f));

   // Set material properties
   shader->setUniform("Material.Kd", 0.9f, 0.5f, 0.3f);  // Diffuse
   shader->setUniform("Material.Ks", 0.8f, 0.8f, 0.8f);  // Specular
   shader->setUniform("Material.Ka", 0.9f, 0.5f, 0.3f);  // Ambient
   shader->setUniform("Material.Shininess", 1.0f);

   // Set other required uniforms
   shader->setUniform("useAO", false);
   shader->setUniform("useSpecular", false);
   shader->setUniform("DrawSkyBox", false);
   shader->setUniform("ReflectFactor", 0.3f);
   shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

   // Render the sphere using the VBO
   if (_sphere)
   {
      // Use the same shader properties for the sphere
      shader->setUniform("MVP", mvp_matrix);
      shader->setUniform("ModelViewMatrix", mv_matrix);
      shader->setUniform("ModelMatrix", model_matrix);
      shader->setUniform("NormalMatrix", normal_matrix);
      shader->setUniform("ProjectionMatrix", projection_matrix);

      // Use different material properties for the sphere to distinguish it
      shader->setUniform("Material.Kd", 0.6f, 0.8f, 1.0f);  // Blueish diffuse for sphere
      shader->setUniform("Material.Ks", 0.8f, 0.8f, 0.8f);
      shader->setUniform("Material.Ka", 0.6f, 0.8f, 1.0f);
      shader->setUniform("Material.Shininess", 50.0f);

      // Set other required uniforms
      shader->setUniform("useAO", false);
      shader->setUniform("useSpecular", false);
      shader->setUniform("DrawSkyBox", false);
      shader->setUniform("ReflectFactor", 0.3f);
      shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

      _sphere->render();
   }

   // Render the starmap textured mesh
   if (_starmapMesh && _starmapTextureId != 0)
   {
      // Calculate model matrix for starmap (positioned next to the sphere)
      glm::mat4 starmap_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f)); // Position to the right
      starmap_model_matrix = glm::rotate(starmap_model_matrix, _starmapRotation, glm::vec3(0.0f, 1.0f, 0.0f)); // Add rotation
      starmap_model_matrix = glm::scale(starmap_model_matrix, glm::vec3(0.5f, 0.5f, 0.5f)); // Scale down

      // Calculate MVP matrices for starmap
      glm::mat4 starmap_mv_matrix = view_matrix * starmap_model_matrix;
      glm::mat4 starmap_mvp_matrix = projection_matrix * starmap_mv_matrix;
      glm::mat3 starmap_normal_matrix = glm::mat3(
         glm::vec3(starmap_mv_matrix[0]),
         glm::vec3(starmap_mv_matrix[1]),
         glm::vec3(starmap_mv_matrix[2])
      );

      // Set the uniforms for the starmap mesh
      shader->setUniform("MVP", starmap_mvp_matrix);
      shader->setUniform("ModelViewMatrix", starmap_mv_matrix);
      shader->setUniform("ModelMatrix", starmap_model_matrix);
      shader->setUniform("NormalMatrix", starmap_normal_matrix);
      shader->setUniform("ProjectionMatrix", projection_matrix);

      // Set material properties for textured mesh
      shader->setUniform("Material.Kd", 1.0f, 1.0f, 1.0f);  // Use texture for diffuse
      shader->setUniform("Material.Ks", 0.5f, 0.5f, 0.5f);
      shader->setUniform("Material.Ka", 0.3f, 0.3f, 0.3f);
      shader->setUniform("Material.Shininess", 32.0f);

      // Set other uniforms
      shader->setUniform("useAO", false);
      shader->setUniform("useSpecular", false);
      shader->setUniform("DrawSkyBox", false);
      shader->setUniform("ReflectFactor", 0.3f);
      shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

      // Bind the starmap texture to texture unit 0
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _starmapTextureId);

      // Tell the shader which texture unit we're using
      shader->setUniform("Tex1", 0);

      _starmapMesh->render();
   }

   // Disable depth testing so ImGui renders correctly on top
   glDisable(GL_DEPTH_TEST);
}

void TestMechanism::update(const sf::Time& dt)
{
   // Update rotation based on rotation speed and time delta
   _currentRotation += _rotationSpeed * dt.asSeconds();
   _starmapRotation += _rotationSpeed * 0.7f * dt.asSeconds(); // Slightly different rotation speed for variety
}

void TestMechanism::resize(int width, int height)
{
   if (_camera)
   {
      _camera->initialize(width, height);  // This updates the projection matrix with new aspect ratio
   }
   glViewport(0, 0, width, height);
}

TestMechanism::~TestMechanism()
{
   // Clean up the texture if it was created
   if (_starmapTextureId != 0) {
      glDeleteTextures(1, &_starmapTextureId);
      _starmapTextureId = 0;
   }
}
