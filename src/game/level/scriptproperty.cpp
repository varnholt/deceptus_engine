#include "scriptproperty.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

using json = nlohmann::json;

void to_json(json& j, const ScriptProperty& p)
{
   j = json{{"name", p._name}, {"value", p._value}};
}

void from_json(const json& j, ScriptProperty& p)
{
   p._name = j.at("name").get<std::string>();
   p._value = j.at("value").get<std::string>();
}
