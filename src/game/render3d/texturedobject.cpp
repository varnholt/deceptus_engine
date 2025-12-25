#include "texturedobject.h"
#include <iostream>
#include "opengl/image/tgaio.h"

TexturedObject::TexturedObject(
   const std::string& objFile,
   const std::string& textureFile,
   float scale,
   bool reCenterMesh,
   bool loadTc,
   bool useLighting
)
    : _mesh(std::make_unique<VBOMesh>(objFile.c_str(), scale, reCenterMesh, loadTc)), _useLighting(useLighting)
{
   loadTexture(textureFile);
}

TexturedObject::~TexturedObject()
{
   if (_textureId != 0)
   {
      glDeleteTextures(1, &_textureId);
      _textureId = 0;
   }
}

void TexturedObject::update(float deltaTime)
{
   _currentRotation += _rotationSpeed * deltaTime;
}

void TexturedObject::render(const std::shared_ptr<GLSLProgram>& shader, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
{
   if (!_mesh || !shader || _textureId == 0)
   {
      return;
   }

   // Create model matrix with position, rotation, and scale
   glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), _position);

   // Apply rotations around X, Y, and Z axes
   model_matrix = glm::rotate(model_matrix, _currentRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));  // X-axis
   model_matrix = glm::rotate(model_matrix, _currentRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));  // Y-axis
   model_matrix = glm::rotate(model_matrix, _currentRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));  // Z-axis

   model_matrix = glm::scale(model_matrix, _scale);

   // Calculate MVP matrices
   glm::mat4 mv_matrix = view_matrix * model_matrix;
   glm::mat4 mvp_matrix = projection_matrix * mv_matrix;
   glm::mat3 normal_matrix = glm::mat3(glm::vec3(mv_matrix[0]), glm::vec3(mv_matrix[1]), glm::vec3(mv_matrix[2]));

   // Set the uniforms for the textured mesh
   shader->setUniform("MVP", mvp_matrix);
   shader->setUniform("ModelViewMatrix", mv_matrix);
   shader->setUniform("ModelMatrix", model_matrix);
   shader->setUniform("NormalMatrix", normal_matrix);

   // Set material properties for textured mesh
   shader->setUniform("Material.Kd", 1.0f, 1.0f, 1.0f);  // Use texture for diffuse
   shader->setUniform("Material.Ks", 0.5f, 0.5f, 0.5f);
   shader->setUniform("Material.Ka", 0.3f, 0.3f, 0.3f);
   shader->setUniform("Material.Shininess", 32.0f);

   // Set the lighting uniform
   shader->setUniform("useLighting", _useLighting);

   // Bind the texture to texture unit 0
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _textureId);

   // Tell the shader which texture unit we're using
   shader->setUniform("Tex1", 0);

   _mesh->render();
}

void TexturedObject::loadTexture(const std::string& textureFile)
{
   if (_textureId != 0)
   {
      glDeleteTextures(1, &_textureId);
      _textureId = 0;
   }

   GLint w, h;
   GLubyte* textureData = TGAIO::read(textureFile.c_str(), w, h);
   if (textureData)
   {
      glGenTextures(1, &_textureId);
      glBindTexture(GL_TEXTURE_2D, _textureId);
      glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);

      delete[] textureData;
   }
   else
   {
      std::cerr << "Failed to load texture: " << textureFile << std::endl;
   }
}
