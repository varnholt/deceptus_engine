#include "gamemechanismdeserializerregistry.h"

GameMechanismDeserializerRegistry& GameMechanismDeserializerRegistry::instance()
{
   static GameMechanismDeserializerRegistry registry;
   return registry;
}

void GameMechanismDeserializerRegistry::registerLayer(const std::string& key, DeserializerFunction deserializer_function)
{
   _layer_map[key] = std::move(deserializer_function);
}

void GameMechanismDeserializerRegistry::registerTemplateType(const std::string& key, DeserializerFunction deserializer_function)
{
   _template_map[key] = std::move(deserializer_function);
}

std::optional<GameMechanismDeserializerRegistry::DeserializerFunction> GameMechanismDeserializerRegistry::getForTemplateType(
   const std::string& key
) const
{
   if (auto it = _template_map.find(key); it != _template_map.end())
   {
      return it->second;
   }

   return std::nullopt;
}

std::optional<GameMechanismDeserializerRegistry::DeserializerFunction> GameMechanismDeserializerRegistry::getForLayer(const std::string& key
) const
{
   if (auto it = _layer_map.find(key); it != _layer_map.end())
   {
      return it->second;
   }

   return std::nullopt;
}
