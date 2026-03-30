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

/// \brief renders and updates the animated 3D menu background scene.
class MenuBackgroundScene
{
public:
   /// \brief creates camera, starmap object and required shader resources.
   MenuBackgroundScene();

   /// \brief releases owned scene object references.
   ~MenuBackgroundScene();

   /// \brief updates all scene objects with frame delta time.
   /// \param deltaTime elapsed time since last update.
   void update(const sf::Time& deltaTime);

   /// \brief renders all scene objects to the provided render target.
   /// \param target SFML render target providing viewport dimensions.
   void render(sf::RenderTarget& target);

   /// \brief appends an object to the scene render/update list.
   /// \param object scene object to add.
   void addObject(std::shared_ptr<Object3D> object);

   /// \brief removes all currently registered scene objects.
   void clear3DObjects();

private:
   std::unique_ptr<Camera3D> _camera;
   std::vector<std::shared_ptr<Object3D>> _objects;
   std::shared_ptr<TexturedObject> _starmap;
   std::shared_ptr<class GLSLProgram> _shader;
};
