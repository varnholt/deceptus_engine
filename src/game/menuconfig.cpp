#include "menuconfig.h"

#include <fstream>
#include "json/json.hpp"

MenuConfig::MenuConfig()
{
   std::ifstream file("data/config/menus.json");
   nlohmann::json j;
   file >> j;

   _duration_hide = FloatSeconds(j["duration_hide"].get<float>());
   _duration_show = FloatSeconds(j["duration_show"].get<float>());
}
