#pragma once

#include <string>

#include "json/json.hpp"

///
/// \brief Base helper for loading, parsing, and writing JSON-backed configuration data.
///
class JsonConfiguration
{
public:
   JsonConfiguration() = default;

protected:
   ///
   /// \brief Reads a file and forwards its contents to deserialize().
   /// \param filename Input JSON file path.
   ///
   void deserializeFromFile(const std::string& filename);
   ///
   /// \brief Serializes current state and writes it to file.
   /// \param filename Output JSON file path.
   ///
   void serializeToFile(const std::string& filename);

   ///
   /// \brief Parses a JSON string into a nlohmann::json object.
   /// \param data JSON text.
   /// \return Parsed JSON object, or empty object on parse failure.
   ///
   nlohmann::json toJson(const std::string& data);
   ///
   /// \brief Formats a JSON object as an indented string.
   /// \param config JSON object to format.
   /// \return Pretty-printed JSON string.
   ///
   std::string toString(const nlohmann::json config);

   ///
   /// \brief Converts derived configuration state to JSON text.
   /// \return Serialized JSON text.
   ///
   virtual std::string serialize();
   ///
   /// \brief Applies JSON text to derived configuration state.
   /// \param data JSON text to consume.
   ///
   virtual void deserialize(const std::string& data);
};
