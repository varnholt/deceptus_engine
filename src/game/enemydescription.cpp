#include "enemydescription.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

using json = nlohmann::json;


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

   if (j.find("id") != j.end())
   {
      d.mId = j.at("id").get<std::string>();
   }

   if (j.find("startposition") != j.end())
   {
      d.mStartPosition = j.at("startposition").get<std::vector<int>>();
   }

   if (j.find("patrolpath") != j.end())
   {
      d.mPatrolPath = j.at("patrolpath").get<std::vector<int>>();

      // allow patrol path to be just defined by a single position
      // i.e. the start position and one new position defines the
      // whole patrol path.
      if (d.mPatrolPath.size() == 2)
      {
         d.mPatrolPath.insert(d.mPatrolPath.begin(), d.mStartPosition[1]);
         d.mPatrolPath.insert(d.mPatrolPath.begin(), d.mStartPosition[0]);
      }
   }

   if (j.find("properties") != j.end())
   {
      d.mProperties = j.at("properties").get<std::vector<ScriptProperty>>();
   }
}

