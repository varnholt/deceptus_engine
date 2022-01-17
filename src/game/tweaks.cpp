#include "tweaks.h"

namespace
{
std::string filename = "data/config/tweaks.json";
}


const Tweaks& Tweaks::instance()
{
   static Tweaks __instance;

   if (!__instance.initialized)
   {
      __instance.initialized = true;
      // __instance.serializeToFile(filename);
      __instance.deserializeFromFile(filename);
   }

   return __instance;
}


std::string Tweaks::serialize()
{
   nlohmann::json config =
   {
      {
         "Tweaks",
         {
            {"bend_down_threshold",    _bend_down_threshold},
            {"cpan_tolerance_x",       _cpan_tolerance_x},
            {"cpan_tolerance_y",       _cpan_tolerance_y},
            {"enter_portal_threshold", _enter_portal_threshold},
         }
      }
   };

   return toString(config);
}


void Tweaks::deserialize(const std::string& data)
{
   auto config = toJson(data);
   _bend_down_threshold    = config["Tweaks"]["bend_down_threshold"].get<float>();
   _cpan_tolerance_x       = config["Tweaks"]["cpan_tolerance_x"].get<float>();
   _cpan_tolerance_y       = config["Tweaks"]["cpan_tolerance_y"].get<float>();
   _enter_portal_threshold = config["Tweaks"]["enter_portal_threshold"].get<float>();
}

