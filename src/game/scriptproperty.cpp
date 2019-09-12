#include "scriptproperty.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

using json = nlohmann::json;


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


