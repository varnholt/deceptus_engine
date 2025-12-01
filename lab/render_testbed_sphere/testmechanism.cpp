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

   float camera_position_arr[3] = {camera_position.x, camera_position.y, camera_position.z};
   float look_at_position_arr[3] = {look_at_position.x, look_at_position.y, look_at_position.z};

   if (ImGui::SliderFloat3("Position", camera_position_arr, -20.0f, 20.0f))
   {
      _camera->setCameraPosition(glm::vec3(camera_position_arr[0], camera_position_arr[1], camera_position_arr[2]));
   }

   if (ImGui::SliderFloat3("Target", look_at_position_arr, -20.0f, 20.0f))
   {
      _camera->setLookAtPoint(glm::vec3(look_at_position_arr[0], look_at_position_arr[1], look_at_position_arr[2]));
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
   glViewport(0, 0, width, height);
}

TestMechanism::~TestMechanism()
{
   _objects.clear();
}
