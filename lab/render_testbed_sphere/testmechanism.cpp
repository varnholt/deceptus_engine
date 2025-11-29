#include "testmechanism.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include "game/shaderpool.h"
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/gl_current.h"

TestMechanism::TestMechanism()
{
   load();
}

void TestMechanism::load()
{
   // Initialize the camera with perspective projection (aspect ratio handled properly)
   _camera = &Camera::getInstance();
   _camera->initialize(1280, 720, -10, 10);  // Initialize with window size - aspect ratio will be handled

   // Set initial camera position and look-at point
   glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);  // Move camera back so we can see the object
   _camera->setCameraPosition(cameraPos);
   _camera->setLookAtPoint(glm::vec3(0.0f, 0.0f, 0.0f));  // Look at origin where the starmap is

   // // Create a sphere object
   // auto sphere = std::make_unique<SphereObject>(1.0f, 50, 50);
   // sphere->setPosition(glm::vec3(-1.5f, 0.0f, 0.0f));  // Position to the left
   // sphere->setRotationSpeed(0.5f);
   // _objects.push_back(std::move(sphere));

   // Create a textured starmap object
   auto starmap = std::make_unique<TexturedObject>(
      "data/objects/starmap.obj",
      "data/textures/starmap_color.tga",
      1.0f,
      true,
      true,
      false  // Disable lighting for starmap to use texture-only color
   );

   starmap->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));  // Position to the right
   starmap->setScale(glm::vec3(1.0f, 1.0f, 1.0f));     // Scale down
   starmap->setRotationSpeed(glm::vec3(0.2f, 0.35f, 0.4f));
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

   if (_objects.size() >= 1) {
      // Assuming the starmap is the first object
      auto* starmap = dynamic_cast<TexturedObject*>(_objects[0].get());

      if (starmap)
      {
         // Starmap controls in a separate group
         ImGui::Text("Starmap Settings");
         ImGui::Separator();

         // Position controls
         glm::vec3 starmapPos = starmap->getPosition();
         float pos[3] = { starmapPos.x, starmapPos.y, starmapPos.z };
         if (ImGui::SliderFloat3("Position", pos, -10.0f, 10.0f)) {
            starmap->setPosition(glm::vec3(pos[0], pos[1], pos[2]));
         }

         // Scale controls
         glm::vec3 starmapScale = starmap->getScale();
         float scale[3] = { starmapScale.x, starmapScale.y, starmapScale.z };
         if (ImGui::SliderFloat3("Scale", scale, 0.1f, 5.0f)) {
            starmap->setScale(glm::vec3(scale[0], scale[1], scale[2]));
         }

         // Rotation speed controls
         glm::vec3 starmapRotationSpeed = starmap->getRotationSpeed();
         float rotationSpeed[3] = { starmapRotationSpeed.x, starmapRotationSpeed.y, starmapRotationSpeed.z };
         if (ImGui::SliderFloat3("Rotation Speed", rotationSpeed, 0.0f, 2.0f, "%.2f")) {
            starmap->setRotationSpeed(glm::vec3(rotationSpeed[0], rotationSpeed[1], rotationSpeed[2]));
         }
      }
   }

   // Camera controls
   if (_camera)
   {
      ImGui::Separator();
      ImGui::Text("Camera Settings");
      ImGui::Separator();

      // Camera position
      glm::vec3 cameraPos = _camera->getCameraPosition();
      float camPos[3] = { cameraPos.x, cameraPos.y, cameraPos.z };
      if (ImGui::SliderFloat3("Camera Position", camPos, -20.0f, 20.0f)) {
         _camera->setCameraPosition(glm::vec3(camPos[0], camPos[1], camPos[2]));
      }

      // Camera look-at point
      glm::vec3 lookAtPoint = _camera->getLookAtPoint();
      float lookAt[3] = { lookAtPoint.x, lookAtPoint.y, lookAtPoint.z };
      if (ImGui::SliderFloat3("Look At Point", lookAt, -20.0f, 20.0f)) {
         _camera->setLookAtPoint(glm::vec3(lookAt[0], lookAt[1], lookAt[2]));
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

   auto* starmap = dynamic_cast<TexturedObject*>(_objects[0].get());
   if (starmap)
   {
      // const auto a = 1000;  //_elapsed.asSeconds();
      // std::cout << a << std::endl;
      // starmap->setScale({a, a, a});
      // starmap->setPosition(glm::vec3(0, 0, a));  // Scale down
   }

   _elapsed += dt;
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
