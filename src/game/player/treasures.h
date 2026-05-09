#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "json/json.hpp"

/// \brief shared treasure definitions loaded once from a JSON file.
namespace TreasureDefinitions
{

/// \brief describes one treasure with its display text.
struct Definition
{
   std::string _identifier;   //!< unique identifier key
   std::string _name;         //!< translated display name
   std::string _description;  //!< translated description
};

/// \brief loads treasure definitions from a JSON file.
///
/// safe to call multiple times; only the first call reads the file.
/// \param filename path to the treasures JSON definition file.
void loadDefinitions(const std::string& filename);

/// \brief returns the definition for a given identifier when found.
/// \param identifier treasure identifier to look up.
/// \return matching definition, or std::nullopt when not found.
std::optional<Definition> findDefinition(const std::string& identifier);

/// \brief returns the full map of all loaded treasure definitions.
/// \return constant reference to the definition map keyed by identifier.
const std::unordered_map<std::string, Definition>& getDefinitions();

}  // namespace TreasureDefinitions

/// \brief per-save tracking of collected treasure identifiers.
struct Treasures
{
   /// \brief marks a treasure as collected when not already collected.
   /// \param identifier identifier of the treasure to collect.
   /// \return true when newly collected, false when already present.
   bool add(const std::string& identifier);

   /// \brief checks whether a treasure has been collected.
   /// \param identifier identifier of the treasure to check.
   /// \return true when the identifier is in the collected list.
   bool has(const std::string& identifier) const;

   /// \brief returns all collected treasure identifiers in insertion order.
   /// \return constant reference to the collected identifier list.
   const std::vector<std::string>& getCollected() const;

   std::vector<std::string> _collected;  //!< ordered list of collected treasure identifiers
};

/// \brief serializes collected treasure identifiers to json.
/// \param j destination json object.
/// \param data source treasures instance.
void to_json(nlohmann::json& j, const Treasures& data);

/// \brief deserializes collected treasure identifiers from json.
/// \param j source json object.
/// \param data destination treasures instance.
void from_json(const nlohmann::json& j, Treasures& data);
