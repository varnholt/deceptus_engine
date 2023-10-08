#include "tweaks.h"

namespace
{
std::string filename = "data/config/tweaks.json";
}

Tweaks& Tweaks::instance()
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
   nlohmann::json config = {
      {"Tweaks",
       {
          {"bend_down_threshold", _bend_down_threshold},
          {"cpan_tolerance_x", _cpan_tolerance_x},
          {"cpan_tolerance_y", _cpan_tolerance_y},
          {"cpan_max_distance_px", _cpan_max_distance_px},
          {"cpan_look_speed_x", _cpan_look_speed_x},
          {"cpan_look_speed_y", _cpan_look_speed_y},
          {"cpan_snap_back_factor", _cpan_snap_back_factor},
          {"enter_portal_threshold", _enter_portal_threshold},
          {"player_light_enabled", _player_light_enabled},
          {"player_light_alpha", _player_light_alpha},
       }}};

   return toString(config);
}

void Tweaks::deserialize(const std::string& data)
{
   auto config = toJson(data);
   _bend_down_threshold = config["Tweaks"]["bend_down_threshold"].get<float>();
   _cpan_tolerance_x = config["Tweaks"]["cpan_tolerance_x"].get<float>();
   _cpan_tolerance_y = config["Tweaks"]["cpan_tolerance_y"].get<float>();
   _cpan_max_distance_px = config["Tweaks"]["cpan_max_distance_px"].get<float>();
   _cpan_look_speed_x = config["Tweaks"]["cpan_look_speed_x"].get<float>();
   _cpan_look_speed_y = config["Tweaks"]["cpan_look_speed_y"].get<float>();
   _cpan_snap_back_factor = config["Tweaks"]["cpan_snap_back_factor"].get<float>();
   _enter_portal_threshold = config["Tweaks"]["enter_portal_threshold"].get<float>();

   if (config.find("player_light_enabled") != config.end())
   {
      _player_light_enabled = config.at("player_light_enabled").get<bool>();
   }

   if (config.find("player_light_alpha") != config.end())
   {
      _player_light_alpha = config.at("player_light_alpha").get<uint8_t>();
   }
}
