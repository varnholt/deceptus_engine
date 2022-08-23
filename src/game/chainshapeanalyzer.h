#ifndef CHAINSHAPEANALYZER_H
#define CHAINSHAPEANALYZER_H

#include <vector>
#include "Box2D/Box2D.h"
#include "constants.h"

namespace ChainShapeAnalyzer
{
void analyze(const std::shared_ptr<b2World>& world);
bool checkPlayerAtCollisionPosition();
bool checkPlayerHiccup();
b2Vec2 lastGoodPosition();
};

#endif // CHAINSHAPEANALYZER_H
