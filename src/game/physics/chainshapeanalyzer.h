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
/// \brief scans static chain fixtures and records shared vertices between incompatible object types.
/// \param world world to inspect for chain-shape fixture conflicts.
void analyze(const std::shared_ptr<b2World>& world);

/// \brief checks whether the player's foot sensor currently overlaps a recorded conflicting vertex.
/// \return conflicting position in meters when the player stands on one, otherwise empty.
std::optional<b2Vec2> checkPlayerAtCollisionPosition();

/// \brief reports whether the player hiccup condition is currently detected by the analyzer.
/// \return true if a hiccup state is detected.
bool checkPlayerHiccup();

/// \brief returns the last stored player position that was considered stable by this analyzer.
/// \return last stable player position in meters.
b2Vec2 lastGoodPosition();
};  // namespace ChainShapeAnalyzer

#endif  // CHAINSHAPEANALYZER_H
