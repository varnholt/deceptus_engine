#pragma once

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <vector>
#include "../glm/glm.hpp"
#include <string>


class VBOMesh : public Drawable
{

public:

   VBOMesh(
      const char * fileName,
      float scale = 1.0f,
      bool reCenterMesh = false,
      bool loadTc = false,
      bool genTangents = false
   );

   void render() const override;
   void loadObj(const char * fileName);


private:

   struct Vertex
   {
      int pIndex;
      int nIndex;
      int tcIndex;
   };

   void trimString( std::string & str );

   void storeVBO2(const std::vector<glm::vec3>& points,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& texCoords,
      const std::vector<glm::vec4>& tangents,
      const std::vector<GLuint>& elements,
      const std::vector<Vertex>& vertices
   );

   void generateAveragedNormals(
      const std::vector<glm::vec3>& points,
      std::vector<glm::vec3>& normals,
      const std::vector<GLuint>& faces
   );

   void generateTangents(
      const std::vector<glm::vec3>& points,
      const std::vector<glm::vec3>& normals,
      const std::vector<GLuint>& faces,
      const std::vector<glm::vec2>& texCoords,
      std::vector<glm::vec4>& tangents
   );

   void center(std::vector<glm::vec3>&);

   float _scale;
   GLuint _faces;
   GLuint _vao_handle;

   bool _recenter_mesh;
   bool _load_texture;
   bool _generate_tangents;
};

