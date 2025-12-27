#include "texturedobject.h"
#include <iostream>
#include "opengl/image/tgaio.h"

TexturedObject::TexturedObject(
   const std::string& objFile,
   const std::string& textureFile,
   float scale,
   bool recenter_mesh,
   bool load_texture_coordinates,
   bool use_lighting
)
    : _mesh(std::make_unique<VBOMesh>(objFile.c_str(), scale, recenter_mesh, load_texture_coordinates)), _use_lighting(use_lighting)
{
   loadTexture(textureFile);
}

TexturedObject::~TexturedObject()
{
   if (_texture_id != 0)
   {
      glDeleteTextures(1, &_texture_id);
      _texture_id = 0;
   }
}

void TexturedObject::update(float deltaTime)
{
   _current_rotation += _rotation_speed * deltaTime;
}

void TexturedObject::render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
{
   if (!_mesh || !shader || _texture_id == 0)
   {
      return;
   }

   // create model matrix with position, rotation, and scale
   glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), _position);
   model_matrix = glm::rotate(model_matrix, _current_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));  // X-axis
   model_matrix = glm::rotate(model_matrix, _current_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));  // Y-axis
   model_matrix = glm::rotate(model_matrix, _current_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));  // Z-axis
   model_matrix = glm::scale(model_matrix, _scale);

   // calculate MVP matrices
   glm::mat4 mv_matrix = view_matrix * model_matrix;
   glm::mat4 mvp_matrix = projection_matrix * mv_matrix;
   glm::mat3 normal_matrix = glm::mat3(glm::vec3(mv_matrix[0]), glm::vec3(mv_matrix[1]), glm::vec3(mv_matrix[2]));

   // transform uniforms
   shader->setUniform("MVP", mvp_matrix);
   shader->setUniform("ModelViewMatrix", mv_matrix);
   shader->setUniform("ModelMatrix", model_matrix);
   shader->setUniform("NormalMatrix", normal_matrix);

   // material properties
   shader->setUniform("Material.Kd", 1.0f, 1.0f, 1.0f);
   shader->setUniform("Material.Ks", 0.5f, 0.5f, 0.5f);
   shader->setUniform("Material.Ka", 0.3f, 0.3f, 0.3f);
   shader->setUniform("Material.Shininess", 32.0f);
   shader->setUniform("useLighting", _use_lighting);

   // bind texture
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture_id);
   shader->setUniform("Tex1", 0);

   _mesh->render();
}

void TexturedObject::setRotationSpeed(const vec3& speed)
{
   _rotation_speed = speed;
}

void TexturedObject::setUseLighting(bool use)
{
   _use_lighting = use;
}

bool TexturedObject::getUseLighting() const
{
   return _use_lighting;
}

vec3 TexturedObject::getRotationSpeed() const
{
   return _rotation_speed;
}

void TexturedObject::loadTexture(const std::string& texture_file_path)
{
   if (_texture_id != 0)
   {
      glDeleteTextures(1, &_texture_id);
      _texture_id = 0;
   }

   GLint w, h;
   GLubyte* texture_data = TGAIO::read(texture_file_path.c_str(), w, h);
   if (texture_data)
   {
      glGenTextures(1, &_texture_id);
      glBindTexture(GL_TEXTURE_2D, _texture_id);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);

      delete[] texture_data;
   }
   else
   {
      std::cerr << "Failed to load texture: " << texture_file_path << std::endl;
   }
}
