#ifndef CHAINSHAPEANALYZER_H
#define CHAINSHAPEANALYZER_H

#include <vector>
#include "Box2D/Box2D.h"

namespace ChainShapeAnalyzer
{
void analyze(std::vector<std::vector<b2Vec2>>& chains);
void analyze(const std::shared_ptr<b2World>& world);
};

#endif // CHAINSHAPEANALYZER_H
