#pragma once

#include "opengl/gl_current.h"  // include GLEW first

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <memory>
#include <vector>

#include "game/render3d/camera3d.h"
#include "game/render3d/object3d.h"
#include "game/render3d/texturedobject.h"
#include "opengl/glslprogram.h"

class MenuBackgroundScene
{
public:
   MenuBackgroundScene();
   ~MenuBackgroundScene();

   void update(const sf::Time& deltaTime);
   void render(sf::RenderTarget& target);

   void addObject(std::shared_ptr<Object3D> object);
   void clear3DObjects();

private:
   void setupOpenGLState();
   void restoreOpenGLState();

   std::unique_ptr<Camera3D> _camera;
   std::vector<std::shared_ptr<Object3D>> _objects;
   std::shared_ptr<TexturedObject> _starmap;
   std::shared_ptr<class GLSLProgram> _shader;
};
