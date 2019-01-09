#include "progresssettings.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;

ProgressSettings ProgressSettings::sInstance;

std::string ProgressSettings::serialize()
{
   // create a JSON value with different types
   json config =
   {
      {
         "ProgressSettings",
         {
            {"level_name", mLevel},
            {"level_checkpoint", mCheckpoint},
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
   mLevel  = config["ProgressSettings"]["level_name"].get<std::string>();
   mCheckpoint = config["ProgressSettings"]["level_checkpoint"].get<int>();
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

