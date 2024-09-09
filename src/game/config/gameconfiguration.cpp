#include "gameconfiguration.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include "framework/tools/log.h"
#include "json/json.hpp"

using json = nlohmann::json;

bool GameConfiguration::__initialized = false;
GameConfiguration GameConfiguration::__defaults;

std::string GameConfiguration::serialize()
{
   json config = {
      {"GameConfiguration",
       {
          {"video_mode_width", _video_mode_width},
          {"video_mode_height", _video_mode_height},
          {"view_width", _view_width},
          {"view_height", _view_height},
          {"fullscreen", _fullscreen},
          {"brightness", _brightness},
          {"vsync", _vsync_enabled},

          {"audio_volume_master", _audio_volume_master},
          {"audio_volume_sfx", _audio_volume_sfx},
          {"audio_volume_music", _audio_volume_music},

          {"text_speed", _text_speed},
          {"rumble", _rumble_enabled},
          {"pause_mode", _pause_mode},
       }}
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

void GameConfiguration::deserialize(const std::string& data)
{
   json config = json::parse(data);

   try
   {
      _video_mode_width = config["GameConfiguration"]["video_mode_width"].get<int32_t>();
      _video_mode_height = config["GameConfiguration"]["video_mode_height"].get<int32_t>();
      _view_width = config["GameConfiguration"]["view_width"].get<int32_t>();
      _view_height = config["GameConfiguration"]["view_height"].get<int32_t>();
      _fullscreen = config["GameConfiguration"]["fullscreen"].get<bool>();
      _brightness = config["GameConfiguration"]["brightness"].get<float>();
      _vsync_enabled = config["GameConfiguration"]["vsync"].get<bool>();

      _video_mode_width = std::max(_video_mode_width, 640);
      _video_mode_height = std::max(_video_mode_height, 360);

      _view_scale_width = static_cast<float>(_view_width) / static_cast<float>(_video_mode_width);
      _view_scale_height = static_cast<float>(_view_height) / static_cast<float>(_video_mode_height);

      _audio_volume_master = config["GameConfiguration"]["audio_volume_master"].get<int32_t>();
      _audio_volume_sfx = config["GameConfiguration"]["audio_volume_sfx"].get<int32_t>();
      _audio_volume_music = config["GameConfiguration"]["audio_volume_music"].get<int32_t>();

      _text_speed = config["GameConfiguration"]["text_speed"].get<int32_t>();
      _rumble_enabled = config["GameConfiguration"]["rumble"].get<bool>();
      _pause_mode = static_cast<PauseMode>(config["GameConfiguration"]["pause_mode"].get<int32_t>());
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }
}

void GameConfiguration::deserializeFromFile(const std::string& filename)
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

void GameConfiguration::serializeToFile(const std::string& filename)
{
   std::string data = serialize();
   std::ofstream file(filename);
   file << data;
}

GameConfiguration& GameConfiguration::getDefaults()
{
   return __defaults;
}

GameConfiguration& GameConfiguration::getInstance()
{
   static GameConfiguration __instance;

   if (!__initialized)
   {
      __instance.deserializeFromFile();
      __initialized = true;
   }

   return __instance;
}

void GameConfiguration::resetAudioDefaults()
{
   getInstance()._audio_volume_master = getDefaults()._audio_volume_master;
   getInstance()._audio_volume_music = getDefaults()._audio_volume_music;
   getInstance()._audio_volume_sfx = getDefaults()._audio_volume_sfx;
}
