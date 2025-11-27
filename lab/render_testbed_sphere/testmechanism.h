#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

// Include OpenGL and related headers from shmup project
#include "vbos/vbosphere.h"
#include "game/camera.h"
#include "opengl/glslprogram.h"

class TestMechanism
{
public:
   TestMechanism();
   virtual ~TestMechanism() = default;
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);
   void resize(int width, int height);
   void drawEditor();

private:
   void load();

   // 3D components
   std::unique_ptr<VBOSphere> _sphere;
   float _rotationSpeed{0.5f};  // Rotation speed in radians per second
   float _currentRotation{0.0f};

   // Camera
   Camera* _camera;

   // For proper cleanup of OpenGL resources
   bool _initialized{false};
};
