#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "framework/tmxparser/tmxelement.h"
#include "framework/tmxparser/tmxparser.h"
#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

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
