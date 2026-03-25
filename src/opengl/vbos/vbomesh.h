#pragma once

#include "opengl/interfaces/drawable.h"

#include "opengl/gl_current.h"

#include "opengl/glm/glm.hpp"

#include <string>
#include <vector>

/// \brief obj mesh loader and renderer that stores geometry in OpenGL buffers.
class VBOMesh : public Drawable
{
public:
   /// \brief loads an obj mesh and uploads its data into a VAO-backed VBO set.
   /// \param filename path to obj file.
   /// \param scale scale factor applied to loaded positions.
   /// \param recenter_mesh whether to center loaded vertices around origin.
   /// \param load_texture_coordinates whether to parse texture coordinates.
   /// \param generate_tangents whether to derive tangent vectors from UVs.
   VBOMesh(
      const char* filename,
      float scale = 1.0f,
      bool recenter_mesh = false,
      bool load_texture_coordinates = false,
      bool generate_tangents = false
   );

   /// \brief draws the uploaded mesh as indexed triangles.
   void render() const override;

   /// \brief loads or reloads mesh data from an obj file and recreates buffers.
   /// \param fileName path to obj file.
   void loadObj(const char* fileName);

private:
   struct Vertex
   {
      int pos_index{};
      int normal_index{};
      int texcoord_index{};
   };

   void storeVbo(
      const std::vector<glm::vec3>& points,
      const std::vector<glm::vec3>& normals,
      const std::vector<glm::vec2>& texcoords,
      const std::vector<glm::vec4>& tangents,
      const std::vector<GLuint>& elements,
      const std::vector<Vertex>& vertices
   );

   float _scale{1.0f};
   GLuint _faces{};
   GLuint _vao_handle{};

   bool _recenter_mesh{false};
   bool _load_texture{false};
   bool _generate_tangents{false};
};
