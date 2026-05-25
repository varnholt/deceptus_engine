#ifndef GAMEMECHANISMDESERIALIZERREGISTRY_H
#define GAMEMECHANISMDESERIALIZERREGISTRY_H

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/gamemechanismschema.h"

class GameNode;

/// \brief stores callback mappings that construct mechanisms from tmx layers and object groups.
class GameMechanismDeserializerRegistry
{
public:
   using DeserializerFunction = std::function<
      void(GameNode*, const GameDeserializeData&, std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>&)>;

   /// \brief returns the global deserializer registry instance.
   /// \return singleton registry used by mechanism registration code.
   static GameMechanismDeserializerRegistry& instance();

   /// \brief registers a deserializer callback for an exact tmx layer name.
   /// \param key layer name used as lookup key.
   /// \param fn callback that creates mechanisms for the layer.
   void registerLayerName(const std::string& key, DeserializerFunction fn);

   /// \brief registers a deserializer callback for an object-group or template key.
   /// \param key object-group key used as lookup key.
   /// \param fn callback that creates mechanisms for matching objects.
   void registerObjectGroup(const std::string& key, DeserializerFunction fn);

   /// \brief links an object-group key and a layer key for cross-lookup.
   /// \param group_name object-group key.
   /// \param layer_name layer-name key.
   void mapGroupToLayer(const std::string& group_name, const std::string& layer_name);

   /// \brief resolves the layer key mapped to a group key.
   /// \param group_name object-group key to resolve.
   /// \return mapped layer key when a mapping exists.
   std::optional<std::string> getLayerName(const std::string& group_name);

   /// \brief resolves the group key mapped to a layer key.
   /// \param layer_name layer key to resolve.
   /// \return mapped object-group key when a mapping exists.
   std::optional<std::string> getObjectGroupName(const std::string& layer_name);

   /// \brief returns the callback registered for a layer key.
   /// \param key layer key to query.
   /// \return callback for the layer when one is registered.
   std::optional<DeserializerFunction> getForLayerName(const std::string& key) const;

   /// \brief returns the callback registered for an object-group key.
   /// \param key object-group key to query.
   /// \return callback for the object group when one is registered.
   std::optional<DeserializerFunction> getForObjectGroup(const std::string& key) const;

   /// \brief exposes the internal layer-to-callback registry for diagnostics.
   /// \return map of layer keys to deserializer callbacks.
   const std::unordered_map<std::string, DeserializerFunction>& getLayerNameMap() const;

   /// \brief exposes the internal group-to-callback registry for diagnostics.
   /// \return map of object-group keys to deserializer callbacks.
   const std::unordered_map<std::string, DeserializerFunction>& getObjectGroupMap() const;

   /// \brief marks a layer as non-visual so that z=0 warnings are suppressed for it.
   /// \param layer_name layer key to mark.
   void markAsNonVisual(const std::string& layer_name);

   /// \brief checks whether a layer is expected to produce visible draw output.
   /// \param layer_name layer key to query.
   /// \return false when the layer was registered as non-visual.
   bool isVisual(const std::string& layer_name) const;

   /// \brief registers a schema describing a mechanism's tmx properties.
   /// \param schema schema to store.
   void registerSchema(const MechanismSchema& schema);

   /// \brief returns all registered mechanism schemas.
   /// \return vector of all schemas registered so far.
   const std::vector<MechanismSchema>& schemas() const;

private:
   std::unordered_map<std::string, DeserializerFunction> _layer_name_map;
   std::unordered_map<std::string, DeserializerFunction> _object_group_map;

   std::map<std::string, std::string> _group_to_layer_name;
   std::map<std::string, std::string> _layer_to_group_name;

   std::unordered_set<std::string> _non_visual_layers;
   std::vector<MechanismSchema> _schemas;
};

#endif  // GAMEMECHANISMDESERIALIZERREGISTRY_H
