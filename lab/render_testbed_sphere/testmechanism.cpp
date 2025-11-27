#include "testmechanism.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/OpenGL.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/gl_current.h"

// Include the shader system from shmup
#include "game/shaderpool.h"

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

   // Initialize the simple shader if not already done
   auto& shaderPool = ShaderPool::getInstance();

   // Check if the simple shader already exists, if not, add it
   if (!shaderPool.get("simple"))
   {
      shaderPool.add("simple", "data/shaders/simple.vs", "data/shaders/simple.fs");
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

   // Get the simple shader from the shader pool
   const auto& shader = ShaderPool::getInstance().get("simple");
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

   // Set the MVP matrix uniform
   shader->setUniform("MVP", mvp_matrix);

   // Set color uniform
   shader->setUniform("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

   // Render the sphere using the VBO
   if (_sphere)
   {
      _sphere->render();
   }

   // Disable depth testing so ImGui renders correctly on top
   glDisable(GL_DEPTH_TEST);
}

void TestMechanism::update(const sf::Time& dt)
{
   // Update rotation based on rotation speed and time delta
   _currentRotation += _rotationSpeed * dt.asSeconds();
}

void TestMechanism::resize(int width, int height)
{
   if (_camera)
   {
      _camera->initialize(width, height);  // This updates the projection matrix with new aspect ratio
   }
   glViewport(0, 0, width, height);
}
