#pragma once

#include "opengl/glm/glm.hpp"

class Transformable
{
public:
   Transformable() = default;
   const glm::mat4& getModelMatrix() const;

protected:
   glm::mat4 _model_matrix;
};
