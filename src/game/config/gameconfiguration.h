#pragma once

#include <cstdint>
#include <string>

/// \brief stores global game settings and handles json persistence.
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
   bool _rumble_enabled = true;

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

   /// \brief loads configuration values from a json file.
   /// \param filename source configuration file path.
   void deserializeFromFile(const std::string& filename = "data/config/game.json");

   /// \brief writes current configuration values to a json file.
   /// \param filename destination configuration file path.
   void serializeToFile(const std::string& filename = "data/config/game.json");

   /// \brief returns the built-in default configuration values.
   /// \return shared default configuration object.
   static GameConfiguration& getDefaults();

   /// \brief returns the active game configuration, loading from disk on first access.
   /// \return singleton runtime configuration object.
   static GameConfiguration& getInstance();

   /// \brief restores all runtime audio volume settings to their default values.
   static void resetAudioDefaults();

   /// \brief checks if a resolution change is significant enough to warrant window recreation.
   /// \param new_width proposed new width in pixels.
   /// \param new_height proposed new height in pixels.
   /// \return true if the size change is significant and should be applied.
   bool isResolutionChangeApplicable(int32_t new_width, int32_t new_height) const;

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
