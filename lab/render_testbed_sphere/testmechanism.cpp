#include "testmechanism.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "game/shaderpool.h"
#include "glm/gtc/matrix_transform.hpp"
#include "opengl/gl_current.h"

TestMechanism::TestMechanism()
{
   load();

   // Set default values after initialization
   _defaultStarmapPosition = _starmap->getPosition();
   _defaultStarmapScale = _starmap->getScale();
   _defaultStarmapRotationSpeed = _starmap->getRotationSpeed();
   _defaultCameraPosition = _camera->getCameraPosition();
   _defaultLookAtPoint = _camera->getLookAtPoint();
   _defaultFOV = _camera->getFOV();
}

void TestMechanism::load()
{
   _camera = &Camera::getInstance();
   _camera->initialize(1280, 720, -10, 10);

   // move camera back so we can see the starmap
   glm::vec3 camera_pos(0.0f, 0.0f, 5.0f);
   _camera->setCameraPosition(camera_pos);

   // look at origin where the starmap is
   _camera->setLookAtPoint(glm::vec3(0.0f, 0.0f, 0.0f));

   // create textured starmap
   _starmap = std::make_shared<TexturedObject>(
      "data/objects/starmap.obj",
      "data/textures/starmap_color.tga",
      1.0f,
      true,
      true,
      false  // disable lighting for starmap to use texture-only color
   );

   _starmap->setRotationSpeed(glm::vec3(0.02f, 0.035f, 0.04f));
   _objects.push_back(_starmap);

   // initialize required shaders
   auto& shader_pool = ShaderPool::getInstance();
   shader_pool.add("texture", "data/shaders/texture.vs", "data/shaders/texture.fs");

   _initialized = true;
}

void TestMechanism::drawEditor()
{
   ImGui::Begin("3D Objects Settings");

   // Add save and reset buttons at the top
   if (ImGui::Button("Save Settings"))
   {
      saveValues();
   }
   ImGui::SameLine();
   if (ImGui::Button("Load Saved"))
   {
      loadValues();
   }
   ImGui::SameLine();
   if (ImGui::Button("Reset to Defaults"))
   {
      resetToDefaults();
   }

   // starmap controls
   ImGui::Text("Starmap Settings");
   ImGui::Separator();

   const auto starmap_position = _starmap->getPosition();
   const auto starmap_scale = _starmap->getScale();
   const auto starmap_rotation_speed = _starmap->getRotationSpeed();

   float starmap_position_arr[3] = {starmap_position.x, starmap_position.y, starmap_position.z};
   float starmap_scale_arr[3] = {starmap_scale.x, starmap_scale.y, starmap_scale.z};
   float starmap_rotation_speed_arr[3] = {starmap_rotation_speed.x, starmap_rotation_speed.y, starmap_rotation_speed.z};

   if (ImGui::SliderFloat3("Position", starmap_position_arr, -10.0f, 10.0f))
   {
      _starmap->setPosition(glm::vec3(starmap_position_arr[0], starmap_position_arr[1], starmap_position_arr[2]));
   }

   if (ImGui::SliderFloat3("Scale", starmap_scale_arr, 0.1f, 5.0f))
   {
      _starmap->setScale(glm::vec3(starmap_scale_arr[0], starmap_scale_arr[1], starmap_scale_arr[2]));
   }

   if (ImGui::SliderFloat3("Rotation Speed", starmap_rotation_speed_arr, 0.0f, 2.0f, "%.2f"))
   {
      _starmap->setRotationSpeed(glm::vec3(starmap_rotation_speed_arr[0], starmap_rotation_speed_arr[1], starmap_rotation_speed_arr[2]));
   }

   // camera controls
   ImGui::Separator();
   ImGui::Text("Camera Settings");
   ImGui::Separator();

   const auto camera_position = _camera->getCameraPosition();
   const auto look_at_position = _camera->getLookAtPoint();
   float fov = _camera->getFOV();

   float camera_position_arr[3] = {camera_position.x, camera_position.y, camera_position.z};
   float look_at_position_arr[3] = {look_at_position.x, look_at_position.y, look_at_position.z};

   if (ImGui::SliderFloat3("Source", camera_position_arr, -20.0f, 20.0f))
   {
      _camera->setCameraPosition(glm::vec3(camera_position_arr[0], camera_position_arr[1], camera_position_arr[2]));
   }

   if (ImGui::SliderFloat3("Target", look_at_position_arr, -20.0f, 20.0f))
   {
      _camera->setLookAtPoint(glm::vec3(look_at_position_arr[0], look_at_position_arr[1], look_at_position_arr[2]));
   }

   if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f, "%.1f"))
   {
      _camera->setFOV(fov);
   }

   ImGui::End();
}

void TestMechanism::draw(sf::RenderTarget& target, sf::RenderTarget&)
{
   if (!_initialized)
   {
      return;
   }

   const auto& shader = ShaderPool::getInstance().get("texture");
   if (!shader)
   {
      return;
   }

   shader->use();

   shader->setUniform("Light.Position", glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));
   shader->setUniform("Light.Intensity", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniform("useAO", false);
   shader->setUniform("useSpecular", false);
   shader->setUniform("DrawSkyBox", false);
   shader->setUniform("ReflectFactor", 0.3f);
   shader->setUniform("WorldCameraPosition", _camera->getCameraPosition());

   glm::mat4 view_matrix = _camera->getViewMatrixCopy();
   glm::mat4 projection_matrix = _camera->getProjectionMatrix();

   for (auto& obj : _objects)
   {
      obj->render(shader, view_matrix, projection_matrix);
   }
}

void TestMechanism::update(const sf::Time& delta_time)
{
   const auto delta_time_s = delta_time.asSeconds();

   for (auto& obj : _objects)
   {
      obj->update(delta_time_s);
   }

   _elapsed += delta_time;
}

void TestMechanism::resize(int width, int height)
{
   _camera->initialize(width, height);
}

TestMechanism::~TestMechanism()
{
   _objects.clear();
}

void TestMechanism::saveValues()
{
   std::ofstream file("editor_settings.txt");
   if (!file.is_open())
   {
      std::cout << "Could not open editor_settings.txt for writing\n";
      return;
   }

   // Save starmap values
   const auto starmap_position = _starmap->getPosition();
   const auto starmap_scale = _starmap->getScale();
   const auto starmap_rotation_speed = _starmap->getRotationSpeed();

   file << "starmap_position_x " << starmap_position.x << "\n";
   file << "starmap_position_y " << starmap_position.y << "\n";
   file << "starmap_position_z " << starmap_position.z << "\n";

   file << "starmap_scale_x " << starmap_scale.x << "\n";
   file << "starmap_scale_y " << starmap_scale.y << "\n";
   file << "starmap_scale_z " << starmap_scale.z << "\n";

   file << "starmap_rotation_speed_x " << starmap_rotation_speed.x << "\n";
   file << "starmap_rotation_speed_y " << starmap_rotation_speed.y << "\n";
   file << "starmap_rotation_speed_z " << starmap_rotation_speed.z << "\n";

   // Save camera values
   const auto camera_position = _camera->getCameraPosition();
   const auto look_at_position = _camera->getLookAtPoint();
   float fov = _camera->getFOV();

   file << "camera_position_x " << camera_position.x << "\n";
   file << "camera_position_y " << camera_position.y << "\n";
   file << "camera_position_z " << camera_position.z << "\n";

   file << "camera_look_at_x " << look_at_position.x << "\n";
   file << "camera_look_at_y " << look_at_position.y << "\n";
   file << "camera_look_at_z " << look_at_position.z << "\n";

   file << "camera_fov " << fov << "\n";

   file.close();
   std::cout << "Settings saved to editor_settings.txt\n";
}

void TestMechanism::loadValues()
{
   std::ifstream file("editor_settings.txt");
   if (!file.is_open())
   {
      std::cout << "Could not open editor_settings.txt for reading\n";
      return;
   }

   std::string line;
   std::string key;
   float value;

   // Initialize default values in case they're missing from file
   glm::vec3 starmap_position = _starmap->getPosition();
   glm::vec3 starmap_scale = _starmap->getScale();
   glm::vec3 starmap_rotation_speed = _starmap->getRotationSpeed();

   glm::vec3 camera_position = _camera->getCameraPosition();
   glm::vec3 look_at_position = _camera->getLookAtPoint();
   float fov = _camera->getFOV();

   while (std::getline(file, line))
   {
      std::istringstream iss(line);
      if (iss >> key >> value)
      {
         if (key == "starmap_position_x") starmap_position.x = value;
         else if (key == "starmap_position_y") starmap_position.y = value;
         else if (key == "starmap_position_z") starmap_position.z = value;

         else if (key == "starmap_scale_x") starmap_scale.x = value;
         else if (key == "starmap_scale_y") starmap_scale.y = value;
         else if (key == "starmap_scale_z") starmap_scale.z = value;

         else if (key == "starmap_rotation_speed_x") starmap_rotation_speed.x = value;
         else if (key == "starmap_rotation_speed_y") starmap_rotation_speed.y = value;
         else if (key == "starmap_rotation_speed_z") starmap_rotation_speed.z = value;

         else if (key == "camera_position_x") camera_position.x = value;
         else if (key == "camera_position_y") camera_position.y = value;
         else if (key == "camera_position_z") camera_position.z = value;

         else if (key == "camera_look_at_x") look_at_position.x = value;
         else if (key == "camera_look_at_y") look_at_position.y = value;
         else if (key == "camera_look_at_z") look_at_position.z = value;

         else if (key == "camera_fov") fov = value;
      }
   }

   // Apply loaded values
   _starmap->setPosition(starmap_position);
   _starmap->setScale(starmap_scale);
   _starmap->setRotationSpeed(starmap_rotation_speed);

   _camera->setCameraPosition(camera_position);
   _camera->setLookAtPoint(look_at_position);
   _camera->setFOV(fov);

   file.close();
   std::cout << "Settings loaded from editor_settings.txt\n";
}

void TestMechanism::resetToDefaults()
{
   _starmap->setPosition(_defaultStarmapPosition);
   _starmap->setScale(_defaultStarmapScale);
   _starmap->setRotationSpeed(_defaultStarmapRotationSpeed);

   _camera->setCameraPosition(_defaultCameraPosition);
   _camera->setLookAtPoint(_defaultLookAtPoint);
   _camera->setFOV(_defaultFOV);

   std::cout << "Values reset to defaults\n";
}
