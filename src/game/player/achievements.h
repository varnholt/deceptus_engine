#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "json/json.hpp"

/// \brief shared achievement definitions loaded once from a JSON file.
namespace AchievementDefinitions
{

/// \brief describes one achievement with its display text.
struct Definition
{
   std::string _identifier;   //!< unique identifier key
   std::string _name;         //!< translated display name
   std::string _description;  //!< translated description
};

/// \brief loads achievement definitions from a JSON file.
///
/// safe to call multiple times; only the first call reads the file.
/// \param filename path to the achievements JSON definition file.
void loadDefinitions(const std::string& filename);

/// \brief returns the definition for a given identifier when found.
/// \param identifier achievement identifier to look up.
/// \return matching definition, or std::nullopt when not found.
std::optional<Definition> findDefinition(const std::string& identifier);

/// \brief returns the full map of all loaded achievement definitions.
/// \return constant reference to the definition map keyed by identifier.
const std::unordered_map<std::string, Definition>& getDefinitions();

}  // namespace AchievementDefinitions

/// \brief per-save tracking of earned achievement identifiers.
struct Achievements
{
   /// \brief marks an achievement as earned when not already earned.
   /// \param identifier identifier of the achievement to earn.
   /// \return true when newly earned, false when already present.
   bool add(const std::string& identifier);

   /// \brief checks whether an achievement has been earned.
   /// \param identifier identifier of the achievement to check.
   /// \return true when the identifier is in the earned list.
   bool has(const std::string& identifier) const;

   /// \brief returns all earned achievement identifiers in insertion order.
   /// \return constant reference to the earned identifier list.
   const std::vector<std::string>& getEarned() const;

   std::vector<std::string> _earned;  //!< ordered list of earned achievement identifiers
};

/// \brief serializes earned achievement identifiers to json.
/// \param j destination json object.
/// \param data source achievements instance.
void to_json(nlohmann::json& j, const Achievements& data);

/// \brief deserializes earned achievement identifiers from json.
/// \param j source json object.
/// \param data destination achievements instance.
void from_json(const nlohmann::json& j, Achievements& data);
