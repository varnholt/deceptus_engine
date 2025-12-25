#include "sphereobject.h"

SphereObject::SphereObject(float radius, int slices, int stacks)
    : _sphere(std::make_unique<VBOSphere>(radius, slices, stacks))
{
}

void SphereObject::update(float deltaTime)
{
    _currentRotation += _rotationSpeed * deltaTime;
}

void SphereObject::render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
{
   if (!_sphere || !shader)
   {
      return;
   }

   // Create model matrix with position, rotation, and scale
   glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), _position);
   model_matrix = glm::rotate(model_matrix, _currentRotation + _rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
   model_matrix = glm::scale(model_matrix, _scale);

   // Calculate MVP matrices
   glm::mat4 mv_matrix = view_matrix * model_matrix;
   glm::mat4 mvp_matrix = projection_matrix * mv_matrix;
   glm::mat3 normal_matrix = glm::mat3(glm::vec3(mv_matrix[0]), glm::vec3(mv_matrix[1]), glm::vec3(mv_matrix[2]));

   // Set the uniforms for the sphere
   shader->setUniform("MVP", mvp_matrix);
   shader->setUniform("ModelViewMatrix", mv_matrix);
   shader->setUniform("ModelMatrix", model_matrix);
   shader->setUniform("NormalMatrix", normal_matrix);

   // Use different material properties for the sphere to distinguish it
   shader->setUniform("Material.Kd", 0.6f, 0.8f, 1.0f);  // Blueish diffuse for sphere
   shader->setUniform("Material.Ks", 0.8f, 0.8f, 0.8f);
   shader->setUniform("Material.Ka", 0.6f, 0.8f, 1.0f);
   shader->setUniform("Material.Shininess", 50.0f);

   _sphere->render();
}
