#include "gamemechanismdeserializerregistry.h"

GameMechanismDeserializerRegistry& GameMechanismDeserializerRegistry::instance()
{
   static GameMechanismDeserializerRegistry registry;
   return registry;
}

void GameMechanismDeserializerRegistry::registerLayerName(const std::string& layer_name, DeserializerFunction deserializer_function)
{
   _layer_name_map[layer_name] = std::move(deserializer_function);
}

void GameMechanismDeserializerRegistry::registerObjectGroup(
   const std::string& object_group_name,
   DeserializerFunction deserializer_function
)
{
   _object_group_map[object_group_name] = std::move(deserializer_function);
}

void GameMechanismDeserializerRegistry::mapGroupToLayer(const std::string& group_name, const std::string& layer_name)
{
   _group_to_layer_name[group_name] = layer_name;
   _layer_to_group_name[layer_name] = group_name;
}

std::optional<std::string> GameMechanismDeserializerRegistry::getLayerName(const std::string& group_name)
{
   const auto it = _group_to_layer_name.find(group_name);
   if (it != _group_to_layer_name.end())
   {
      return it->second;
   }
   return std::nullopt;
}

std::optional<std::string> GameMechanismDeserializerRegistry::getObjectGroupName(const std::string& layer_name)
{
   const auto it = _layer_to_group_name.find(layer_name);
   if (it != _layer_to_group_name.end())
   {
      return it->second;
   }
   return std::nullopt;
}

const std::unordered_map<std::string, GameMechanismDeserializerRegistry::DeserializerFunction>&
GameMechanismDeserializerRegistry::getObjectGroupMap() const
{
   return _object_group_map;
}

const std::unordered_map<std::string, GameMechanismDeserializerRegistry::DeserializerFunction>&
GameMechanismDeserializerRegistry::getLayerNameMap() const
{
   return _layer_name_map;
}

std::optional<GameMechanismDeserializerRegistry::DeserializerFunction> GameMechanismDeserializerRegistry::getForObjectGroup(
   const std::string& key
) const
{
   if (auto it = _object_group_map.find(key); it != _object_group_map.end())
   {
      return it->second;
   }

   return std::nullopt;
}

void GameMechanismDeserializerRegistry::markAsNonVisual(const std::string& layer_name)
{
   _non_visual_layers.insert(layer_name);
}

bool GameMechanismDeserializerRegistry::isVisual(const std::string& layer_name) const
{
   return _non_visual_layers.find(layer_name) == _non_visual_layers.end();
}

std::optional<GameMechanismDeserializerRegistry::DeserializerFunction> GameMechanismDeserializerRegistry::getForLayerName(
   const std::string& key
) const
{
   if (auto it = _layer_name_map.find(key); it != _layer_name_map.end())
   {
      return it->second;
   }

   return std::nullopt;
}

void GameMechanismDeserializerRegistry::registerSchema(const MechanismSchema& schema)
{
   _schemas.push_back(schema);
}

const std::vector<MechanismSchema>& GameMechanismDeserializerRegistry::schemas() const
{
   return _schemas;
}
