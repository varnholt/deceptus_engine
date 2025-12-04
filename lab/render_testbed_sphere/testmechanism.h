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
   void saveValues();
   void loadValues();
   void resetToDefaults();

private:
   void load();

   // 3D objects
   std::vector<std::shared_ptr<Object3D>> _objects;
   std::shared_ptr<TexturedObject> _starmap;

   // Camera
   Camera* _camera;

   // For proper cleanup of OpenGL resources
   bool _initialized{false};

   sf::Time _elapsed;

   // Store default values for reset functionality
   glm::vec3 _defaultStarmapPosition{0.0f, 0.0f, 0.0f};
   glm::vec3 _defaultStarmapScale{1.0f, 1.0f, 1.0f};
   glm::vec3 _defaultStarmapRotationSpeed{0.02f, 0.035f, 0.04f};
   glm::vec3 _defaultCameraPosition{0.0f, 0.0f, 5.0f};
   glm::vec3 _defaultLookAtPoint{0.0f, 0.0f, 0.0f};
   float _defaultFOV{70.0f};
};
