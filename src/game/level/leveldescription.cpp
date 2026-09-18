#include "leveldescription.h"

#include "framework/tools/assetsource.h"
#include "framework/tools/log.h"

#include <iostream>
#include <ostream>
#include <sstream>

using json = nlohmann::json;

void to_json(json& j, const LevelDescription& d)
{
   j = json{
      {"filename", d._filename}, {"startposition", d._start_position_tl}, {"enemies", d._enemies}, {"footstep_surface", d._footstep_surface}
   };
}

void from_json(const json& j, LevelDescription& d)
{
   d._filename = j.at("filename").get<std::string>();
   d._start_position_tl = j.at("startposition").get<std::vector<int>>();

   if (j.find("enemies") != j.end())
   {
      d._enemies = j.at("enemies").get<std::vector<EnemyDescription>>();
   }

   if (j.find("footstep_surface") != j.end())
   {
      d._footstep_surface = j.at("footstep_surface").get<std::string>();
   }
}

std::shared_ptr<LevelDescription> LevelDescription::load(const std::string& path)
{
   const auto file_contents = AssetSource::readFile(path);
   if (!file_contents.has_value())
   {
      Log::Error() << "path does not exist: " << path;
      return nullptr;
   }

   std::shared_ptr<LevelDescription> description;
   try
   {
      const json config = json::parse(*file_contents);
      description = std::make_shared<LevelDescription>();
      *description = config;
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }

   return description;
}
