#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "framework/tmxparser/tmxparser.h"
#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/level/gamenode.h"

namespace GameMechanismDeserializer
{

void deserialize(
   const TmxParser& tmx_parser,
   GameNode* parent,
   const GameDeserializeData& data,
   std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>& mechanisms
);

bool isLayerNameReserved(const std::string& layer_name);

}  // namespace GameMechanismDeserializer
