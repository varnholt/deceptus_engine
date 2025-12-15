#ifndef CHAINSHAPEANALYZER_H
#define CHAINSHAPEANALYZER_H

#include "box2d/box2d.h"
#include <memory>
#include <optional>
#include <vector>
#include "constants.h"

//
//
//           O
//          /|\          
//          / \        !!
//       +-------------++--------------+
//       |   chain a   ||    chain b    \
//       +------+      |+-----------+    +
//               \     |             \  /
//                +----+              +/
//
//
// player will jump a bit up between the two adjacent vertices (!!)

namespace ChainShapeAnalyzer
{
void analyze(const std::shared_ptr<b2World>& world);
std::optional<b2Vec2> checkPlayerAtCollisionPosition();
bool checkPlayerHiccup();
b2Vec2 lastGoodPosition();
};  // namespace ChainShapeAnalyzer

#endif  // CHAINSHAPEANALYZER_H
