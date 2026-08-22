#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "framework/tools/gamepaths.h"

/// \brief stores global game settings and handles json persistence.
struct GameConfiguration
{
   int32_t _video_mode_width = 1280;  // size of the actual window, read back after creation, never serialized
   int32_t _video_mode_height = 720;  // size of the actual window, read back after creation, never serialized
   int32_t _windowed_width = 1280;    // windowed mode width (serialized)
   int32_t _windowed_height = 720;    // windowed mode height (serialized)
   int32_t _view_width = 640;
   int32_t _view_height = 360;
   bool _fullscreen = false;

   //!< blits the view at a whole number scale, so one view pixel always covers a whole number of screen
   //!< pixels and nothing is resampled. whatever space is left over in the window becomes bars
   bool _preserve_pixel_precision = false;

   //!< scales both axes by the same factor, so the view keeps its shape instead of being stretched to the
   //!< window. the axis that falls short of the window edge ends up with bars
   bool _preserve_aspect_ratio = false;

   float _view_scale_width = 1.0f;
   float _view_scale_height = 1.0f;
   float _brightness = 0.5f;
   bool _vsync_enabled = true;
   bool _rumble_enabled = true;

   //!< sizes the level's render targets by group, see RenderTargetProfile. "full" keeps every
   //!< target at the size of the window image; "reduced" renders lighting, normals and the
   //!< atmosphere at half size and stretches them back, which costs a quarter of the fragments
   //!< in those passes and leaves the visible image untouched
   std::string _render_target_profile = "full";

   int32_t _audio_volume_master = 100;
   int32_t _audio_volume_sfx = 100;
   int32_t _audio_volume_music = 100;

   enum class PauseMode
   {
      AutomaticPause = 0,
      ManualPause = 1
   };

   int32_t _text_speed = 2;
   PauseMode _pause_mode = PauseMode::AutomaticPause;
   std::string _language = "en";  //!< locale identifier loaded at startup, e.g. "en", "it", "ja"

   /// \brief loads configuration values from a json file.
   /// \param filename source configuration file path.
   void deserializeFromFile(const std::string& filename = GamePaths::getPreferencesFile("game.json").string());

   /// \brief writes current configuration values to a json file.
   /// \param filename destination configuration file path.
   void serializeToFile(const std::string& filename = GamePaths::getPreferencesFile("game.json").string());

   /// \brief largest whole number of screen pixels one view pixel may occupy.
   /// \param video_mode_width width of the target the view is scaled into.
   /// \param video_mode_height height of that target.
   /// \param view_width width of the view, in view pixels.
   /// \param view_height height of the view, in view pixels.
   /// \return the scale, floored to a whole number and never below one.
   /// \note the view is pixel art, so the scale must never be fractional. A window of 2560x1369 has
   ///       a vertical ratio of 3.8, and rasterising the art at 3.8 puts a one pixel eye across 3.8
   ///       screen pixels - it comes out as a smear. Flooring to 3 letterboxes the difference
   ///       instead, and every pixel stays exact.
   static int32_t computeViewScale(int32_t video_mode_width, int32_t video_mode_height, int32_t view_width, int32_t view_height);

   /// \brief the view scale for the configured video mode and view.
   /// \return the scale, floored to a whole number and never below one.
   int32_t getViewScale() const;

   /// \brief where the window render texture goes inside the window, and how big it is drawn.
   struct WindowImagePlacement
   {
      int32_t texture_width = 0;   //!< width of the render texture, always a whole multiple of the view
      int32_t texture_height = 0;  //!< height of the render texture, always a whole multiple of the view
      float scale_x = 1.0f;        //!< horizontal factor the render texture is blitted with
      float scale_y = 1.0f;        //!< vertical factor the render texture is blitted with
      float offset_x = 0.0f;       //!< distance from the left window edge to the left edge of the blit
      float offset_y = 0.0f;       //!< distance from the top window edge to the top edge of the blit

      //!< whether the blit lands view pixels on fractions of a screen pixel. only then is there
      //!< anything between texels to interpolate, and only then is smoothing worth having
      bool resamples = false;
   };

   /// \brief sizes the window render texture and works out the blit that puts it into the window.
   /// \return the texture size together with the scale and offset the blit runs at.
   /// \note the render texture is a whole multiple of the view in every behavior. only the blit differs,
   ///       so switching behavior never resizes a render target or changes what the level draws.
   WindowImagePlacement computeWindowImagePlacement() const;

   /// \brief returns the built-in default configuration values.
   /// \return shared default configuration object.
   static GameConfiguration& getDefaults();

   /// \brief returns the active game configuration, loading from disk on first access.
   /// \return singleton runtime configuration object.
   static GameConfiguration& getInstance();

   /// \brief restores all runtime audio volume settings to their default values.
   static void resetAudioDefaults();

   /// \brief ensures the configured resolution fits within desktop limits.
   /// clamps _windowed_width and _windowed_height to the desktop resolution if needed.
   /// persists changes to disk if the resolution was adjusted.
   void clampResolutionToDesktop();

#ifdef DECEPTUS_VRSFML
   /// \brief computes the largest integer multiple of the base view that fits the browser viewport.
   /// keeping the render resolution an exact multiple of _view_width x _view_height avoids fractional
   /// scaling, which would turn the pixel-art fonts into uneven mush.
   /// \return the video mode width and height as a {width, height} pair.
   std::pair<int32_t, int32_t> computeViewportVideoMode() const;
#endif

private:
   /// \brief serializes the current settings into formatted json text.
   /// \return json string containing the GameConfiguration object.
   std::string serialize();

   /// \brief parses json text and applies known configuration values.
   /// \param data json payload containing a GameConfiguration object.
   void deserialize(const std::string& data);

   /// \brief constructs a configuration object populated with default values.
   GameConfiguration() = default;

   static bool __initialized;
   static GameConfiguration __defaults;
};
