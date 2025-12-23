#pragma once

#include "opengl/gl_current.h"  // Include GLEW first

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "opengl/glslprogram.h"
#include "opengl/vbos/vbomesh.h"
#include "opengl/vbos/vbosphere.h"
#include "game/render3d/3dobject.h"

namespace deceptus
{
namespace render3d
{

// Define StarmapObject as a typedef to TexturedObject for consistency with our naming
class TexturedObject;  // Forward declaration
using StarmapObject = TexturedObject;

class Camera3D
{
public:
   Camera3D();
   virtual ~Camera3D() = default;

   void initialize(int width, int height, float near_plane = 0.1f, float far_plane = 100.0f);
   void update(float deltaTime);

   const glm::mat4& getViewMatrix() const
   {
      return _viewMatrix;
   }
   const glm::mat4& getProjectionMatrix() const
   {
      return _projectionMatrix;
   }
   glm::mat4 getViewMatrixCopy() const
   {
      return _viewMatrix;
   }

   void setCameraPosition(const glm::vec3& pos);
   glm::vec3 getCameraPosition() const { return _cameraPosition; }
   void setLookAtPoint(const glm::vec3& target);
   glm::vec3 getLookAtPoint() const { return _lookAtPoint; }
   void setFOV(float fov)
   {
      _fov = fov;
      _projDirty = true;
   }
   float getFOV() const { return _fov; }

private:
   void updateViewMatrix();
   void updateProjectionMatrix();

   glm::vec3 _cameraPosition{0.0f, 0.0f, 5.0f};
   glm::vec3 _lookAtPoint{0.0f, 0.0f, 0.0f};
   float _fov{70.0f};
   float _near{0.1f};
   float _far{100.0f};
   int _width{800};
   int _height{600};

   glm::mat4 _viewMatrix{1.0f};
   glm::mat4 _projectionMatrix{1.0f};

   bool _viewDirty{true};
   bool _projDirty{true};
};

class Renderer3D
{
public:
   Renderer3D();
   ~Renderer3D();

   void initialize();
   void update(const sf::Time& deltaTime);
   void render(sf::RenderTarget& target);

   void add3DObject(std::shared_ptr<Object3D> object);
   void clear3DObjects();

   // Set camera parameters specifically for starmap rendering
   void setupStarmapCamera();

private:
   void setupOpenGLState();
   void restoreOpenGLState();

   std::unique_ptr<Camera3D> _camera;
   std::vector<std::shared_ptr<Object3D>> _objects;
   std::shared_ptr<class GLSLProgram> _shader;
   bool _initialized{false};
};

}  // namespace render3d
}  // namespace deceptus
