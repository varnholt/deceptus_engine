#include "enemydescription.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

using json = nlohmann::json;


void to_json(json &j, const EnemyDescription &d)
{
   j = json{
      {"script", d._script},
      {"startposition", d._start_position},
      {"path", d._path},
      {"properties", d._properties}
   };
}


void from_json(const json &j, EnemyDescription &d)
{
   d._script = j.at("script").get<std::string>();

   if (j.find("id") != j.end())
   {
      d._id = j.at("id").get<std::string>();
   }

   if (j.find("startposition") != j.end())
   {
      d._start_position = j.at("startposition").get<std::vector<int>>();
   }

   if (j.find("generate_path") != j.end())
   {
      d._generate_path = true;
   }
   else if (j.find("path") != j.end())
   {
      d._path = j.at("path").get<std::vector<int>>();

      // allow patrol path to be just defined by a single position
      // i.e. the start position and one new position defines the
      // whole patrol path.
      if (d._path.size() == 2)
      {
         d._path.insert(d._path.begin(), d._start_position[1]);
         d._path.insert(d._path.begin(), d._start_position[0]);
      }
   }

   if (j.find("properties") != j.end())
   {
      d._properties = j.at("properties").get<std::vector<ScriptProperty>>();
   }
}

