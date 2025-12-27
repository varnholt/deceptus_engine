#include "menubackgroundscene.h"

#include "opengl/glm/gtc/type_ptr.hpp"
#include "opengl/shaderpool.h"

#include <iostream>

MenuBackgroundScene::MenuBackgroundScene()
{
   // Initialize the camera
   _camera = std::make_unique<Camera3D>();
   _camera->initialize(800, 600, -10, 10);       // Use same near/far as lab after correction, will be updated before rendering
   _camera->setPosition({0.0f, 0.0f, 5.0f});     // Position camera along z-axis                                               │
   _camera->setLookAtPoint({0.0f, 0.0f, 0.0f});  // Look at origin where starmap is

   // create textured starmap
   _starmap = std::make_shared<TexturedObject>(
      "data/meshes/starmap.obj",
      "data/effects/starmap_color.tga",
      1.0f,
      true,
      true,
      false  // disable lighting for starmap to use texture-only color
   );

   _starmap->setPosition({0, 0, 0});
   _starmap->setScale({1, 1, 1});
   _starmap->setRotationSpeed(glm::vec3(0.02f, 0.035f, 0.04f));  // Use lab's rotation speed

   addObject(_starmap);

   // initialize required shaders
   auto& shader_pool = ShaderPool::getInstance();
   shader_pool.add("texture", "data/shaders/texture.vs", "data/shaders/texture.fs");

   _shader = shader_pool.get("texture");
   if (!_shader)
   {
      std::cerr << "Failed to load texture shader!\n";
      return;
   }
}

MenuBackgroundScene::~MenuBackgroundScene()
{
   _objects.clear();
}

void MenuBackgroundScene::update(const sf::Time& delta_time)
{
   const auto delta_time_s = delta_time.asSeconds();
   for (auto& obj : _objects)
   {
      obj->update(delta_time_s);
   }
}

void MenuBackgroundScene::render(sf::RenderTarget& target)
{
   if (_objects.empty() || !_shader)
   {
      return;
   }

   // set viewport
   sf::Vector2u target_size = target.getSize();
   glViewport(0, 0, static_cast<GLsizei>(target_size.x), static_cast<GLsizei>(target_size.y));

   // prepare OpenGL state
   setupOpenGLState();

   _shader->use();
   _shader->setUniform("Light.Position", glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));
   _shader->setUniform("Light.Intensity", glm::vec3(1.0f, 1.0f, 1.0f));
   _shader->setUniform("useAO", false);
   _shader->setUniform("useSpecular", false);
   _shader->setUniform("DrawSkyBox", false);
   _shader->setUniform("ReflectFactor", 0.3f);
   _shader->setUniform("WorldCameraPosition", _camera->getPosition());

   // get matrices from camera
   glm::mat4 view_matrix = _camera->getViewMatrix();
   glm::mat4 projection_matrix = _camera->getProjectionMatrix();

   // render all objects
   for (auto& obj : _objects)
   {
      obj->render(_shader, view_matrix, projection_matrix);
   }

   // restore OpenGL state
   restoreOpenGLState();
}

void MenuBackgroundScene::addObject(std::shared_ptr<Object3D> object)
{
   _objects.push_back(object);
}

void MenuBackgroundScene::setupOpenGLState()
{
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MenuBackgroundScene::restoreOpenGLState()
{
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glUseProgram(0);
}

void MenuBackgroundScene::clear3DObjects()
{
   _objects.clear();
}
