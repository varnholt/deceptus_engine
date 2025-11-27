#pragma once

#include "../glm/glm.hpp"
#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <cstdint>
#include <vector>


class VboLineStrip : public Drawable
{
   public:

      enum class Mode
      {
         Strip,
         Loop
      };

      VboLineStrip() = default;
      VboLineStrip(const std::vector<glm::vec3>& positions, Mode mode = Mode::Strip);

      void render() const override;

      uint32_t _vao_handle = 0;
      uint32_t _element_count = 0;
      Mode _mode = Mode::Strip;
};

