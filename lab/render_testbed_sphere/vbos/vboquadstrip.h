#pragma once

#include "interfaces/drawable.h"
#include "opengl/gl_current.h"

#include <vector>
#include "../glm/glm.hpp"

class VboQuadStrip : public Drawable
{
public:

   VboQuadStrip() = default;
   VboQuadStrip(const std::vector<glm::vec3>& positions);

   void render() const override;

protected:

   uint32_t _vao_handle = 0;
   uint32_t _element_count = 0;
};

