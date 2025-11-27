#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

// Include OpenGL and related headers from shmup project
#include "game/camera.h"
#include "opengl/glslprogram.h"
#include "sphereobject.h"
#include "texturedobject.h"

class TestMechanism
{
public:
   TestMechanism();
   virtual ~TestMechanism();
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);
   void resize(int width, int height);
   void drawEditor();

private:
   void load();

   // 3D objects
   std::vector<std::unique_ptr<Object3D>> _objects;

   // Camera
   Camera* _camera;

   // For proper cleanup of OpenGL resources
   bool _initialized{false};
};
