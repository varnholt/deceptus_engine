#include "progresssettings.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;


std::string ProgressSettings::serialize()
{
   // create a JSON value with different types
   json config =
   {
      {
         "ProgressSettings",
         {
            {"level_name", _level},
            {"level_checkpoint", _checkpoint},
         }
      }
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}


void ProgressSettings::deserialize(const std::string& data)
{
   json config = json::parse(data);
   _level  = config["ProgressSettings"]["level_name"].get<std::string>();
   _checkpoint = config["ProgressSettings"]["level_checkpoint"].get<int>();
}


void ProgressSettings::deserializeFromFile(const std::string &filename)
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


void ProgressSettings::serializeToFile(const std::string &filename)
{
  std::string data = serialize();
  std::ofstream file(filename);
  file << data;
}


ProgressSettings& ProgressSettings::getInstance()
{
   static ProgressSettings __instance;
   return __instance;
}

