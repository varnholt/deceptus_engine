#include "gameconfiguration.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include "SFML/Graphics.hpp"
#include "framework/tools/log.h"
#include "json/json.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using json = nlohmann::json;

namespace
{
// minimum pixel difference required to trigger window recreation
// filters out small DPI adjustments when moving windows between monitors
constexpr int32_t min_resolution_change_threshold = 10;
}  // namespace

bool GameConfiguration::__initialized = false;
GameConfiguration GameConfiguration::__defaults;

std::string GameConfiguration::serialize()
{
   json config = {
      {"GameConfiguration",
       {
          {"video_mode_width", _video_mode_width},
          {"video_mode_height", _video_mode_height},
          {"windowed_width", _windowed_width},
          {"windowed_height", _windowed_height},
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
          {"language", _language},
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

      // load windowed dimensions, fallback to video_mode if not present
      const auto& gc = config["GameConfiguration"];
      if (const auto it = gc.find("windowed_width"); it != gc.end())
      {
         _windowed_width = it->get<int32_t>();
         _windowed_height = gc["windowed_height"].get<int32_t>();
      }
      else
      {
         _windowed_width = _video_mode_width;
         _windowed_height = _video_mode_height;
      }

      _view_width = config["GameConfiguration"]["view_width"].get<int32_t>();
      _view_height = config["GameConfiguration"]["view_height"].get<int32_t>();
      _fullscreen = config["GameConfiguration"]["fullscreen"].get<bool>();
      _brightness = config["GameConfiguration"]["brightness"].get<float>();
      _vsync_enabled = config["GameConfiguration"]["vsync"].get<bool>();

      _video_mode_width = std::max(_video_mode_width, 640);
      _video_mode_height = std::max(_video_mode_height, 360);
      _windowed_width = std::max(_windowed_width, 640);
      _windowed_height = std::max(_windowed_height, 360);

      _view_scale_width = static_cast<float>(_view_width) / static_cast<float>(_video_mode_width);
      _view_scale_height = static_cast<float>(_view_height) / static_cast<float>(_video_mode_height);

      _audio_volume_master = config["GameConfiguration"]["audio_volume_master"].get<int32_t>();
      _audio_volume_sfx = config["GameConfiguration"]["audio_volume_sfx"].get<int32_t>();
      _audio_volume_music = config["GameConfiguration"]["audio_volume_music"].get<int32_t>();

      _text_speed = config["GameConfiguration"]["text_speed"].get<int32_t>();
      _rumble_enabled = config["GameConfiguration"]["rumble"].get<bool>();
      _pause_mode = static_cast<PauseMode>(config["GameConfiguration"]["pause_mode"].get<int32_t>());

      if (const auto language_it = gc.find("language"); language_it != gc.end())
      {
         _language = language_it->get<std::string>();
      }
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
      // seed defaults from the actual desktop so first-launch resolution is sensible
#ifndef __EMSCRIPTEN__
      const auto desktop = sf::VideoMode::getDesktopMode();
      __instance._video_mode_width = static_cast<int32_t>(desktop.size.x);
      __instance._video_mode_height = static_cast<int32_t>(desktop.size.y);
#else
      __instance._video_mode_width = 1920;
      __instance._video_mode_height = 1080;
#endif
      __instance._windowed_width = __instance._video_mode_width;
      __instance._windowed_height = __instance._video_mode_height;

      // config file values override the desktop defaults when present
      __instance.deserializeFromFile();

      // if the file was missing or corrupt, windowed dimensions may still be zero
      if (__instance._windowed_width == 0 || __instance._windowed_height == 0)
      {
         __instance._windowed_width = __instance._video_mode_width;
         __instance._windowed_height = __instance._video_mode_height;
      }

#ifdef __EMSCRIPTEN__
      // on the web the canvas fills the browser/itch viewport, whose size is arbitrary. render at the
      // largest integer multiple of the 640x360 base view that fits so pixel-art fonts stay crisp.
      // this overrides the config file's video mode.
      {
         const auto [viewport_video_mode_width, viewport_video_mode_height] = __instance.computeViewportVideoMode();
         __instance._video_mode_width = viewport_video_mode_width;
         __instance._video_mode_height = viewport_video_mode_height;
      }
#endif

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

bool GameConfiguration::isResolutionChangeApplicable(int32_t new_width, int32_t new_height) const
{
   // in fullscreen mode, the OS manages the window size
   if (_fullscreen)
   {
      return false;
   }

   // only apply resolution changes that exceed the threshold
   // this filters out DPI scaling adjustments when moving windows between monitors
   const auto width_diff = std::abs(new_width - _video_mode_width);
   const auto height_diff = std::abs(new_height - _video_mode_height);

   return (width_diff > min_resolution_change_threshold || height_diff > min_resolution_change_threshold);
}

void GameConfiguration::clampResolutionToDesktop()
{
#ifndef __EMSCRIPTEN__
   const auto desktop_mode = sf::VideoMode::getDesktopMode();
   const auto desktop_width = static_cast<int32_t>(desktop_mode.size.x);
   const auto desktop_height = static_cast<int32_t>(desktop_mode.size.y);

   if (_video_mode_width > desktop_width || _video_mode_height > desktop_height)
   {
      Log::Warning() << "configured resolution " << _video_mode_width << "x" << _video_mode_height << " exceeds desktop resolution "
                     << desktop_width << "x" << desktop_height << ", clamping to desktop size";

      _video_mode_width = std::min(_video_mode_width, desktop_width);
      _video_mode_height = std::min(_video_mode_height, desktop_height);
      serializeToFile();
   }
#endif
}

#ifdef __EMSCRIPTEN__
std::pair<int32_t, int32_t> GameConfiguration::computeViewportVideoMode() const
{
   // work in device pixels (css pixels * device pixel ratio), so the render resolution is an integer
   // multiple of the base view in *physical* pixels. on displays with os scaling (dpr != 1, e.g. windows
   // at 125%) the browser upscales the canvas by dpr, so a buffer that is only an integer multiple in css
   // pixels still gets fractionally resampled to physical pixels, turning the pixel-art fonts into mush.
   // the shell then sets the canvas css size to buffer/dpr so this device-pixel buffer maps 1:1 on screen.
   const double device_pixel_ratio = emscripten_get_device_pixel_ratio();
   const auto available_width = static_cast<int32_t>(EM_ASM_DOUBLE({ return window.innerWidth; }) * device_pixel_ratio);
   const auto available_height = static_cast<int32_t>(EM_ASM_DOUBLE({ return window.innerHeight; }) * device_pixel_ratio);
   const auto integer_scale = std::max(1, std::min(available_width / _view_width, available_height / _view_height));
   return {_view_width * integer_scale, _view_height * integer_scale};
}
#endif
