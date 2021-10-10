#include "levels.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "framework/tools/log.h"

using json = nlohmann::json;


void Levels::deserialize(const std::string& data)
{
   try
   {
      json config = json::parse(data);
      _levels = config.get<std::vector<LevelItem>>();
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }
}


void Levels::deserializeFromFile(const std::string &filename)
{
   std::ifstream ifs (filename, std::ifstream::in);

   auto c = ifs.get();
   std::string data;

   while (ifs.good())
   {
      data.push_back(static_cast<char>(c));
      c = ifs.get();
   }

   ifs.close();

   deserialize(data);
}


Levels& Levels::getInstance()
{
   static Levels __instance;
   return __instance;
}


void from_json(const json &j, LevelItem &item)
{
   item.mLevelName = j.at("levelname").get<std::string>();
}
