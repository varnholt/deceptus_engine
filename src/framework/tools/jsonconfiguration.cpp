#include "jsonconfiguration.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

void JsonConfiguration::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   char c = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(c);
      c = static_cast<char>(ifs.get());
   }

   ifs.close();

   deserialize(data);
}

void JsonConfiguration::serializeToFile(const std::string& filename)
{
   std::string data = serialize();
   std::ofstream file(filename);
   file << data;
}

nlohmann::json JsonConfiguration::toJson(const std::string& data)
{
   nlohmann::json config;

   try
   {
      config = nlohmann::json::parse(data);
   }
   catch (const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   return config;
}

std::string JsonConfiguration::toString(const nlohmann::json config)
{
   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

std::string JsonConfiguration::serialize()
{
   return {};
}

void JsonConfiguration::deserialize(const std::string& /*data*/)
{
}
