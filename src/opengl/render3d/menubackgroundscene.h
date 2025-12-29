#pragma once

#include "opengl/gl_current.h"  // include GLEW first

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <memory>
#include <vector>

#include "opengl/glslprogram.h"
#include "opengl/render3d/camera3d.h"
#include "opengl/render3d/object3d.h"
#include "opengl/render3d/texturedobject.h"

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
   std::unique_ptr<Camera3D> _camera;
   std::vector<std::shared_ptr<Object3D>> _objects;
   std::shared_ptr<TexturedObject> _starmap;
   std::shared_ptr<class GLSLProgram> _shader;
};
