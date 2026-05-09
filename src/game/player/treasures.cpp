#include "treasures.h"

#include "framework/tools/localization.h"
#include "framework/tools/log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

namespace TreasureDefinitions
{

namespace
{
std::unordered_map<std::string, Definition> definitions;
}

void loadDefinitions(const std::string& filename)
{
   static bool definitions_loaded = false;
   if (definitions_loaded)
   {
      return;
   }
   definitions_loaded = true;

   if (!std::filesystem::exists(filename))
   {
      Log::Warning() << "treasures definitions file not found: " << filename;
      return;
   }

   std::ifstream file_stream(filename, std::ifstream::in);
   std::string file_data;
   auto character = file_stream.get();
   while (file_stream.good())
   {
      file_data.push_back(static_cast<char>(character));
      character = file_stream.get();
   }
   file_stream.close();

   try
   {
      const auto json_data = json::parse(file_data);
      for (const auto& json_entry : json_data)
      {
         Definition definition;
         definition._identifier = json_entry.at("id").get<std::string>();
         definition._name = tr(json_entry.at("name").get<std::string>());
         definition._description = tr(json_entry.at("description").get<std::string>());
         definitions[definition._identifier] = definition;
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "failed to parse treasures definitions: " << exception.what();
   }
}

std::optional<Definition> findDefinition(const std::string& identifier)
{
   const auto found = definitions.find(identifier);
   if (found == definitions.end())
   {
      return std::nullopt;
   }
   return found->second;
}

const std::unordered_map<std::string, Definition>& getDefinitions()
{
   return definitions;
}

}  // namespace TreasureDefinitions

bool Treasures::add(const std::string& identifier)
{
   if (has(identifier))
   {
      return false;
   }
   _collected.push_back(identifier);
   return true;
}

bool Treasures::has(const std::string& identifier) const
{
   return std::ranges::find(_collected, identifier) != _collected.end();
}

const std::vector<std::string>& Treasures::getCollected() const
{
   return _collected;
}

void to_json(nlohmann::json& j, const Treasures& data)
{
   j = json{{"collected", data.getCollected()}};
}

void from_json(const nlohmann::json& j, Treasures& data)
{
   if (j.find("collected") != j.end())
   {
      for (const auto& identifier : j.at("collected").get<std::vector<std::string>>())
      {
         data.add(identifier);
      }
   }
}
