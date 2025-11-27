#include "testmechanism.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/OpenGL.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/gl_current.h"
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

   // Create a sphere object
   auto sphere = std::make_unique<SphereObject>(1.0f, 50, 50);
   sphere->setPosition(glm::vec3(-1.5f, 0.0f, 0.0f));  // Position to the left
   sphere->setRotationSpeed(0.5f);
   _objects.push_back(std::move(sphere));

   // Create a textured starmap object
   auto starmap = std::make_unique<TexturedObject>(
      "data/objects/starmap.obj",
      "data/textures/starmap_color.tga",
      1.0f,
      true,
      true
   );
   starmap->setPosition(glm::vec3(1.5f, 0.0f, 0.0f));  // Position to the right
   starmap->setScale(glm::vec3(0.5f, 0.5f, 0.5f));     // Scale down
   starmap->setRotationSpeed(0.35f);
   _objects.push_back(std::move(starmap));

   // Initialize the required shaders if not already done
   auto& shaderPool = ShaderPool::getInstance();

   // Check if the texture shader already exists, if not, add it
   if (!shaderPool.get("texture"))
   {
      shaderPool.add("texture", "data/shaders/texture.vs", "data/shaders/texture.fs");
   }

   _initialized = true;
}

void TestMechanism::drawEditor()
{
   ImGui::Begin("3D Objects Settings");

   if (_objects.size() >= 2) {
      auto* sphere = dynamic_cast<SphereObject*>(_objects[0].get());
      auto* starmap = dynamic_cast<TexturedObject*>(_objects[1].get());
      
      if (sphere) {
         float sphereSpeed = sphere->getRotationSpeed();
         if (ImGui::SliderFloat("Sphere Rotation Speed", &sphereSpeed, 0.0f, 2.0f, "%.2f")) {
            sphere->setRotationSpeed(sphereSpeed);
         }
      }
      
      if (starmap) {
         float starmapSpeed = starmap->getRotationSpeed();
         if (ImGui::SliderFloat("Starmap Rotation Speed", &starmapSpeed, 0.0f, 2.0f, "%.2f")) {
            starmap->setRotationSpeed(starmapSpeed);
         }
      }
   }

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

   // Set lighting uniforms (use similar values as in the renderer)
   shader->setUniform("Light.Position", glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));
   shader->setUniform("Light.Intensity", glm::vec3(1.0f, 1.0f, 1.0f));

   // Set other required uniforms
   shader->setUniform("useAO", false);
   shader->setUniform("useSpecular", false);
   shader->setUniform("DrawSkyBox", false);
   shader->setUniform("ReflectFactor", 0.3f);
   shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

   // Get view and projection matrices from camera
   glm::mat4 view_matrix = _camera->getViewMatrixCopy();
   glm::mat4 projection_matrix = _camera->getProjectionMatrix();

   // Render all objects
   for (auto& obj : _objects)
   {
      obj->render(shader, view_matrix, projection_matrix);
   }

   // Disable depth testing so ImGui renders correctly on top
   glDisable(GL_DEPTH_TEST);
}

void TestMechanism::update(const sf::Time& dt)
{
   float deltaTime = dt.asSeconds();
   
   // Update all objects
   for (auto& obj : _objects)
   {
      obj->update(deltaTime);
   }
}

void TestMechanism::resize(int width, int height)
{
   if (_camera) {
      _camera->initialize(width, height);  // This updates the projection matrix with new aspect ratio
   }
   glViewport(0, 0, width, height);
}

TestMechanism::~TestMechanism()
{
   // Objects are automatically cleaned up via unique_ptr
   _objects.clear();
}