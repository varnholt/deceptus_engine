#pragma once

#include "opengl/glm/glm.hpp"

/// \brief base class exposing a model matrix for transformable entities.
class Transformable
{
public:
   Transformable() = default;

   /// \brief returns the current model matrix stored by the object.
   /// \return constant reference to the model matrix.
   const glm::mat4& getModelMatrix() const;

protected:
   glm::mat4 _model_matrix;
};
