#pragma once

#include <cstdint>
#include <string>

struct GameConfiguration
{
   int32_t _video_mode_width = 1280;
   int32_t _video_mode_height = 720;
   int32_t _view_width = 640;
   int32_t _view_height = 360;
   bool _fullscreen = false;
   float _view_scale_width = 1.0f;
   float _view_scale_height = 1.0f;
   float _brightness = 0.5f;
   bool _vsync_enabled = false;

   int32_t _audio_volume_master = 50;
   int32_t _audio_volume_sfx = 50;
   int32_t _audio_volume_music = 50;

   int32_t _text_speed = 2;

   void deserializeFromFile(const std::string& filename = "data/config/game.json");
   void serializeToFile(const std::string& filename = "data/config/game.json");

   static GameConfiguration& getDefaults();
   static GameConfiguration& getInstance();

   static void resetAudioDefaults();

private:

   std::string serialize();
   void deserialize(const std::string& data);

   GameConfiguration() = default;

   static bool __initialized;
   static GameConfiguration __defaults;
};

