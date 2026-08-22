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

bool GameConfiguration::__initialized = false;
GameConfiguration GameConfiguration::__defaults;

std::string GameConfiguration::serialize()
{
   json config = {
      {"GameConfiguration",
       {
          {"windowed_width", _windowed_width},
          {"windowed_height", _windowed_height},
          {"view_width", _view_width},
          {"view_height", _view_height},
          {"fullscreen", _fullscreen},
          {"brightness", _brightness},
          {"vsync", _vsync_enabled},
          {"preserve_pixel_precision", _preserve_pixel_precision},
          {"preserve_aspect_ratio", _preserve_aspect_ratio},
          {"render_target_profile", _render_target_profile},

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
      _windowed_width = config["GameConfiguration"]["windowed_width"].get<int32_t>();
      _windowed_height = config["GameConfiguration"]["windowed_height"].get<int32_t>();
      _view_width = config["GameConfiguration"]["view_width"].get<int32_t>();
      _view_height = config["GameConfiguration"]["view_height"].get<int32_t>();
      _fullscreen = config["GameConfiguration"]["fullscreen"].get<bool>();
      _brightness = config["GameConfiguration"]["brightness"].get<float>();
      _vsync_enabled = config["GameConfiguration"]["vsync"].get<bool>();

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

      const auto& gc = config["GameConfiguration"];
      if (const auto language_it = gc.find("language"); language_it != gc.end())
      {
         _language = language_it->get<std::string>();
      }

      // read through find() rather than operator[]: the whole block is one try, so a key an older
      // settings file does not carry would throw and take every read after it down with it
      if (const auto profile_it = gc.find("render_target_profile"); profile_it != gc.end())
      {
         _render_target_profile = profile_it->get<std::string>();
      }

      if (const auto pixel_precision_it = gc.find("preserve_pixel_precision"); pixel_precision_it != gc.end())
      {
         _preserve_pixel_precision = pixel_precision_it->get<bool>();
      }

      if (const auto aspect_ratio_it = gc.find("preserve_aspect_ratio"); aspect_ratio_it != gc.end())
      {
         _preserve_aspect_ratio = aspect_ratio_it->get<bool>();
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
   file.close();

   GamePaths::flushToPersistentStorage();
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
      // seed defaults from the actual desktop so first-launch resolution is sensible.
      // the switch has no desktop either: it scans out at a fixed 720p handheld or
      // 1080p docked, so it takes the same path as the web build.
#if defined(__SWITCH__)
      // the handheld scan-out size, which is also what the NWindow reports by default.
      // docked mode hands out 1080p, but the window reports its real size at creation
      // time, so starting from 720p avoids allocating render targets for a resolution
      // this session may never run at
      __instance._video_mode_width = 1280;
      __instance._video_mode_height = 720;

      // the console is fragment bound at its scan-out resolution, so the lighting, normal and
      // atmosphere passes run at half size by default here. the config file still overrides it
      __instance._render_target_profile = "reduced";
#elif !defined(__EMSCRIPTEN__)
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

int32_t GameConfiguration::computeViewScale(int32_t video_mode_width, int32_t video_mode_height, int32_t view_width, int32_t view_height)
{
   if (view_width <= 0 || view_height <= 0)
   {
      Log::Warning() << "invalid view dimensions " << view_width << " x " << view_height;
      return 1;
   }

   // integer division is the flooring
   const auto scale_width = video_mode_width / view_width;
   const auto scale_height = video_mode_height / view_height;

   return std::max(1, std::min(scale_width, scale_height));
}

int32_t GameConfiguration::getViewScale() const
{
   return computeViewScale(_video_mode_width, _video_mode_height, _view_width, _view_height);
}

GameConfiguration::WindowImagePlacement GameConfiguration::computeWindowImagePlacement() const
{
   // the render texture is a whole multiple of the view in every mode, not just the pixel precise one.
   // the view is pixel art and rasterising it at a fraction of a pixel is what turns it to mush, so the
   // fractional part of the fit is left to the blit, where it resamples an already finished image
   const auto integer_scale = getViewScale();
   const auto texture_width = integer_scale * _view_width;
   const auto texture_height = integer_scale * _view_height;

   // pixel precision takes precedence when both are on: a whole number scale applied to both axes alike
   // already keeps the aspect ratio, so the two ask for the same thing and this is the stricter of them
   if (_preserve_pixel_precision)
   {
      // whole numbers the entire way through. the offset has to land on a whole pixel as well: a window
      // with an odd amount of space left over puts the true centre at x.5, and blitting a texture there
      // resamples every pixel of the frame - the one thing this mode exists to prevent. integer division
      // floors it instead, which leaves one bar a single pixel wider than the other
      const auto offset_x = (_video_mode_width - texture_width) / 2;
      const auto offset_y = (_video_mode_height - texture_height) / 2;

      return {
         .texture_width = texture_width,
         .texture_height = texture_height,
         .scale_x = 1.0f,
         .scale_y = 1.0f,
         .offset_x = static_cast<float>(offset_x),
         .offset_y = static_cast<float>(offset_y)
      };
   }

   // both remaining modes resample, so here sub-pixel centring is the more accurate answer rather than
   // the thing to avoid, and the arithmetic runs in float
   const auto window_width = static_cast<float>(_video_mode_width);
   const auto window_height = static_cast<float>(_video_mode_height);
   const auto fill_scale_x = window_width / static_cast<float>(texture_width);
   const auto fill_scale_y = window_height / static_cast<float>(texture_height);

   if (!_preserve_aspect_ratio)
   {
      // each axis takes whatever factor reaches its window edge, which fills the window completely and
      // distorts the view by however much the window disagrees with 16:9
      return {
         .texture_width = texture_width,
         .texture_height = texture_height,
         .scale_x = fill_scale_x,
         .scale_y = fill_scale_y,
         .offset_x = 0.0f,
         .offset_y = 0.0f
      };
   }

   // one factor for both axes keeps the view's shape, and taking the smaller of the two keeps the result
   // inside the window. the axis that then falls short of its edge is the one that ends up with bars
   const auto uniform_scale = std::min(fill_scale_x, fill_scale_y);
   const auto image_width = static_cast<float>(texture_width) * uniform_scale;
   const auto image_height = static_cast<float>(texture_height) * uniform_scale;

   return {
      .texture_width = texture_width,
      .texture_height = texture_height,
      .scale_x = uniform_scale,
      .scale_y = uniform_scale,
      .offset_x = (window_width - image_width) * 0.5f,
      .offset_y = (window_height - image_height) * 0.5f
   };
}

void GameConfiguration::resetAudioDefaults()
{
   getInstance()._audio_volume_master = getDefaults()._audio_volume_master;
   getInstance()._audio_volume_music = getDefaults()._audio_volume_music;
   getInstance()._audio_volume_sfx = getDefaults()._audio_volume_sfx;
}

void GameConfiguration::clampResolutionToDesktop()
{
   // nothing to clamp against on the web or on the switch; neither has a desktop
#if !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
   const auto desktop_mode = sf::VideoMode::getDesktopMode();
   const auto desktop_width = static_cast<int32_t>(desktop_mode.size.x);
   const auto desktop_height = static_cast<int32_t>(desktop_mode.size.y);

   if (_windowed_width > desktop_width || _windowed_height > desktop_height)
   {
      Log::Warning() << "configured resolution " << _windowed_width << "x" << _windowed_height << " exceeds desktop resolution "
                     << desktop_width << "x" << desktop_height << ", clamping to desktop size";

      _windowed_width = std::min(_windowed_width, desktop_width);
      _windowed_height = std::min(_windowed_height, desktop_height);
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
