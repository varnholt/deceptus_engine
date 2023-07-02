#include "levels.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "framework/tools/log.h"

using json = nlohmann::json;

namespace
{
std::vector<LevelItem> _levels;

void deserialize(const std::string& data)
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

void deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

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

}  // namespace

LevelItem Levels::readLevelItem(int32_t index)
{
   if (_levels.empty())
   {
      deserializeFromFile("data/config/levels.json");
   }

   if (index < _levels.size())
   {
      return _levels[index];
   }

   return {};
}

std::vector<LevelItem> Levels::readLevelItems()
{
   if (_levels.empty())
   {
      deserializeFromFile("data/config/levels.json");
   }

   return _levels;
}

void from_json(const json& j, LevelItem& item)
{
   item._level_name = j.at("levelname").get<std::string>();
}
