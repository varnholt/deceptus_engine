#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "framework/tmxparser/tmxparser.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

namespace GameMechanismDeserializer
{

/// \brief builds all mechanism instances from parsed tmx layers and object groups.
/// \param tmx_parser parsed tmx source with layers, groups, and objects.
/// \param parent parent node that receives created mechanisms.
/// \param data shared deserialize context with world and resource paths.
/// \param mechanisms output registry buckets keyed by mechanism layer name.
void deserialize(
   const TmxParser& tmx_parser,
   GameNode* parent,
   const GameDeserializeData& data,
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms
);

/// \brief checks whether a layer name is reserved for legacy hard-coded deserialization paths.
/// \param layer_name tmx layer name to test.
/// \return true when the layer name is handled by reserved mechanism logic.
bool isLayerNameReserved(const std::string& layer_name);

}  // namespace GameMechanismDeserializer
