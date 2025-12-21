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
#include "menu3dobject.h"

namespace deceptus
{
namespace menu3d
{

// Define StarmapObject as a typedef to TexturedObject for consistency with our naming
class TexturedObject;  // Forward declaration
using StarmapObject = TexturedObject;

class Menu3DCamera
{
public:
   Menu3DCamera();
   virtual ~Menu3DCamera() = default;

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

class Menu3DRenderer
{
public:
   Menu3DRenderer();
   ~Menu3DRenderer();

   void initialize();
   void update(const sf::Time& deltaTime);
   void render(sf::RenderTarget& target);

   void add3DObject(std::shared_ptr<Menu3DObject> object);
   void clear3DObjects();

private:
   void setupOpenGLState();
   void restoreOpenGLState();

   std::unique_ptr<Menu3DCamera> _camera;
   std::vector<std::shared_ptr<Menu3DObject>> _objects;
   std::shared_ptr<class GLSLProgram> _shader;
   bool _initialized{false};
};

}  // namespace menu3d
}  // namespace deceptus