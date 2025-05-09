#ifndef GAMEMECHANISMDESERIALIZERREGISTRY_H
#define GAMEMECHANISMDESERIALIZERREGISTRY_H

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "game/io/gamedeserializedata.h"
#include "game/mechanisms/gamemechanism.h"

class GameNode;

class GameMechanismDeserializerRegistry
{
public:
   using DeserializerFunction = std::function<
      void(GameNode*, const GameDeserializeData&, std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>&)>;

   static GameMechanismDeserializerRegistry& instance();

   void registerLayer(const std::string& key, DeserializerFunction fn);
   void registerTemplateType(const std::string& key, DeserializerFunction fn);

   std::optional<DeserializerFunction> getForLayer(const std::string& key) const;
   std::optional<DeserializerFunction> getForTemplateType(const std::string& key) const;

private:
   std::unordered_map<std::string, DeserializerFunction> _layer_map;
   std::unordered_map<std::string, DeserializerFunction> _template_map;
};

#endif  // GAMEMECHANISMDESERIALIZERREGISTRY_H
