#include "levels.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;

Levels Levels::sInstance;


void Levels::deserialize(const std::string& data)
{
   json config = json::parse(data);

   try
   {
     mLevels = config.get<std::vector<LevelItem>>();
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
   }
}


void Levels::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

  char c = ifs.get();
  std::string data;

  while (ifs.good())
  {
    // std::cout << c;
    data.push_back(c);
    c = ifs.get();
  }

  ifs.close();

  deserialize(data);
}


void from_json(const json &j, LevelItem &item)
{
  item.mLevelName = j.at("levelname").get<std::string>();
}
