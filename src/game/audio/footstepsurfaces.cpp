#include "footstepsurfaces.h"

#include "framework/tools/log.h"
#include "game/audio/audio.h"

#include "json/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

namespace FootstepSurfaces
{

namespace
{
std::unordered_map<std::string, Definition> definitions;
std::string default_surface{"stone"};
}  // namespace

std::optional<std::string> Definition::pickSample(bool left) const
{
   const auto& samples = left ? _left : _right;

   if (samples.empty())
   {
      return std::nullopt;
   }

   return samples[std::rand() % samples.size()];
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
      Log::Warning() << "footstep definitions file not found: " << filename;
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

      default_surface = json_data.at("default_surface").get<std::string>();

      for (const auto& json_entry : json_data.at("surfaces"))
      {
         Definition definition;
         definition._surface = json_entry.at("surface").get<std::string>();
         definition._volume = json_entry.at("volume").get<float>();
         definition._left = json_entry.at("left").get<std::vector<std::string>>();
         definition._right = json_entry.at("right").get<std::vector<std::string>>();

         for (const auto& sample : definition._left)
         {
            Audio::getInstance().addSample(sample);
         }

         for (const auto& sample : definition._right)
         {
            Audio::getInstance().addSample(sample);
         }

         definitions[definition._surface] = definition;
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "failed to parse footstep definitions: " << exception.what();
   }
}

std::optional<Definition> findDefinition(const std::string& surface)
{
   const auto found = definitions.find(surface);
   if (found == definitions.end())
   {
      return std::nullopt;
   }
   return found->second;
}

const std::string& getDefaultSurface()
{
   return default_surface;
}

}  // namespace FootstepSurfaces
