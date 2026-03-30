#pragma once

#include "../glm/glm.hpp"
#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <cstdint>
#include <vector>

/// \brief polyline renderer using indexed GL_LINE_STRIP or GL_LINE_LOOP draw calls.
class VboLineStrip : public Drawable
{
public:

   /// \brief selects whether endpoints are connected in the rendered line.
   enum class Mode
   {
      Strip,
      Loop
   };

   VboLineStrip() = default;

   /// \brief uploads line vertices and sequential indices.
   /// \param positions ordered polyline vertex positions.
   /// \param mode strip for open polyline, loop for closed polyline.
   VboLineStrip(const std::vector<glm::vec3>& positions, Mode mode = Mode::Strip);

   /// \brief draws the line strip/loop using the configured mode.
   void render() const override;

   uint32_t _vao_handle = 0;
   uint32_t _element_count = 0;
   Mode _mode = Mode::Strip;
};
