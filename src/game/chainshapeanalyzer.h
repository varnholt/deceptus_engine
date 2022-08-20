#ifndef CHAINSHAPEANALYZER_H
#define CHAINSHAPEANALYZER_H

#include <vector>
#include "Box2D/Box2D.h"
#include "constants.h"

namespace ChainShapeAnalyzer
{
void analyze(std::vector<std::vector<b2Vec2>>& chains);
void analyze(const std::shared_ptr<b2World>& world);
void fix(b2ChainShape* chain, int32_t vertex_index, ObjectType object_type, bool right);
};

#endif // CHAINSHAPEANALYZER_H
