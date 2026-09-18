#include "achievements.h"

#include "framework/tools/assetsource.h"
#include "framework/tools/localization.h"
#include "framework/tools/log.h"

#include <algorithm>

using json = nlohmann::json;

namespace AchievementDefinitions
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

   const auto file_data = AssetSource::readFile(filename);
   if (!file_data.has_value())
   {
      Log::Warning() << "achievements definitions file not found: " << filename;
      return;
   }

   try
   {
      const auto json_data = json::parse(*file_data);
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
      Log::Error() << "failed to parse achievements definitions: " << exception.what();
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

}  // namespace AchievementDefinitions

bool Achievements::add(const std::string& identifier)
{
   if (has(identifier))
   {
      return false;
   }
   _earned.push_back(identifier);
   return true;
}

bool Achievements::has(const std::string& identifier) const
{
   return std::ranges::find(_earned, identifier) != _earned.end();
}

const std::vector<std::string>& Achievements::getEarned() const
{
   return _earned;
}

void to_json(nlohmann::json& j, const Achievements& data)
{
   j = json{{"earned", data.getEarned()}};
}

void from_json(const nlohmann::json& j, Achievements& data)
{
   if (j.find("earned") != j.end())
   {
      for (const auto& identifier : j.at("earned").get<std::vector<std::string>>())
      {
         data.add(identifier);
      }
   }
}
