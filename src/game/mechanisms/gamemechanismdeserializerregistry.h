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

   void registerLayerName(const std::string& key, DeserializerFunction fn);
   void registerObjectGroup(const std::string& key, DeserializerFunction fn);
   void mapGroupToLayer(const std::string& group_name, const std::string& layer_name);
   std::optional<std::string> getLayerName(const std::string& group_name);
   std::optional<std::string> getObjectGroupName(const std::string& layer_name);

   std::optional<DeserializerFunction> getForLayerName(const std::string& key) const;
   std::optional<DeserializerFunction> getForObjectGroup(const std::string& key) const;

   const std::unordered_map<std::string, DeserializerFunction>& getLayerNameMap() const;
   const std::unordered_map<std::string, DeserializerFunction>& getObjectGroupMap() const;

private:
   std::unordered_map<std::string, DeserializerFunction> _layer_name_map;
   std::unordered_map<std::string, DeserializerFunction> _object_group_map;

   std::map<std::string, std::string> _group_to_layer_name;
   std::map<std::string, std::string> _layer_to_group_name;
};

#endif  // GAMEMECHANISMDESERIALIZERREGISTRY_H
