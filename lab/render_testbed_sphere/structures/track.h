#pragma once

#include "glm/glm.hpp"

#include "interfaces/transformable.h"
#include "vbos/vbolinestrip.h"
#include "vbos/vboquadstrip.h"

#include <string>
#include <vector>

class Track : public Transformable
{
   struct Vertex {
      uint32_t pIndex = 0;
      uint32_t nIndex = 0;
      uint32_t tcIndex = 0;
   };

public:
   Track() = default;
   void deserialize();
   void createVbo();

   const VboQuadStrip& getVbo() const;
   const std::vector<glm::vec3>& getRingInner() const;
   const std::vector<glm::vec3>& getRingOuter() const;
   const std::vector<glm::vec3>& getQuadPositions() const;
   const std::vector<glm::vec3>& getPoints() const;

   void render();

private:
   void deserialize(
         const std::string& filename,
      std::vector<glm::vec3>& points,
      std::vector<std::vector<uint32_t>>& faces
   );

   std::vector<glm::vec3> _points;
   std::vector<std::vector<uint32_t>> _faces;

   std::vector<glm::vec3> _positions;
   std::vector<glm::vec3> _ring_inner;
   std::vector<glm::vec3> _ring_outer;

   VboQuadStrip _vbo_track_quads;
   VboLineStrip _vbo_track_border_1;
   VboLineStrip _vbo_track_border_2;

   void scale();
};

