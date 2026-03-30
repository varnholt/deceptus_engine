#pragma once

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <vector>
#include "../glm/glm.hpp"

/// \brief mesh builder for a quad strip represented as connected triangle pairs.
class VboQuadStrip : public Drawable
{
public:
   VboQuadStrip() = default;

   /// \brief builds quad-strip triangle indices from paired edge vertices.
   /// \param positions ordered point pairs defining strip cross-sections.
   VboQuadStrip(const std::vector<glm::vec3>& positions);

   /// \brief draws generated quad strip triangles.
   void render() const override;

protected:

   uint32_t _vao_handle = 0;
   uint32_t _element_count = 0;
};

