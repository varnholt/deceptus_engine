#include "transformable.h"


const glm::mat4& Transformable::getModelMatrix() const
{
   return _model_matrix;
}
