#include "leveldescription.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>


void to_json(json &j, const ScriptProperty &p)
{
   j = json{
      {"name", p.mName},
      {"value", p.mValue}
   };
}


void from_json(const json &j, ScriptProperty &p)
{
   p.mName = j.at("name").get<std::string>();
   p.mValue = j.at("value").get<std::string>();
}


void to_json(json &j, const EnemyDescription &d)
{
   j = json{
      {"script", d.mScript},
      {"startposition", d.mStartPosition},
      {"patrolpath", d.mPatrolPath},
      {"properties", d.mProperties}
   };
}


void from_json(const json &j, EnemyDescription &d)
{
   d.mScript = j.at("script").get<std::string>();
   d.mStartPosition = j.at("startposition").get<std::vector<int>>();
   if (j.find("patrolpath") != j.end())
      d.mPatrolPath = j.at("patrolpath").get<std::vector<int>>();
   d.mProperties = j.at("properties").get<std::vector<ScriptProperty>>();
}


void to_json(json &j, const LevelDescription &d)
{
   j = json{
      {"filename", d.mFilename},
      {"startposition", d.mStartPosition},
      {"enemies", d.mEnemies}
   };
}


void from_json(const json &j, LevelDescription &d)
{
   d.mFilename = j.at("filename").get<std::string>();
   d.mStartPosition = j.at("startposition").get<std::vector<int>>();
   if (j.find("enemies") != j.end())
   d.mEnemies = j.at("enemies").get<std::vector<EnemyDescription>>();
}


std::shared_ptr<LevelDescription> LevelDescription::load(const std::string &path)
{
   std::ifstream ifs (path, std::ifstream::in);

   char c = ifs.get();
   std::string data;

   while (ifs.good())
   {
     data.push_back(c);
     c = ifs.get();
   }

   ifs.close();

   std::shared_ptr<LevelDescription> description;

   try
   {
     json config = json::parse(data);
     description = std::make_shared<LevelDescription>();
     *description = config;
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
   }

   return description;
}


