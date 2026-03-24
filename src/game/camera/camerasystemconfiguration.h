#pragma once

#include <cstdint>
#include <string>

/// \brief stores runtime camera tuning values and serializes them to json.
struct CameraSystemConfiguration
{
   /// \brief returns the global camera configuration, loading it from file on first access.
   /// \return singleton configuration instance.
   static CameraSystemConfiguration& getInstance();

   /// \brief returns horizontal camera interpolation speed multiplier.
   /// \return velocity factor applied during x-axis camera updates.
   float getCameraVelocityFactorX() const;

   /// \brief returns the divisor used to derive horizontal focus-zone half-width from view width.
   /// \return focus-zone divider value.
   float getFocusZoneDivider() const;

   /// \brief returns how far the focus zone shifts based on player orientation.
   /// \return shift factor multiplied by focus-zone half-range.
   float getTargetShiftFactor() const;

   /// \brief returns horizontal deadband used to clear focus-zone triggering.
   /// \return x-axis tolerance in pixels.
   int32_t getBackInBoundsToleranceX() const;

   /// \brief checks whether camera focus shifts with player facing direction.
   /// \return true when orientation-based target shifting is enabled.
   bool isFollowingPlayerOrientation() const;

   /// \brief returns vertical camera interpolation speed multiplier.
   /// \return velocity factor applied during y-axis camera updates.
   float getCameraVelocityFactorY() const;

   /// \brief returns the divisor used to place upper and lower panic lines.
   /// \return panic-line divider value.
   float getPanicLineDivider() const;

   /// \brief returns the vertical anchor ratio used for camera top-left conversion.
   /// \return view ratio used by CameraSystem::getY().
   float getViewRatioY() const;

   /// \brief returns vertical deadband used to clear focus and panic follow state.
   /// \return y-axis tolerance in pixels.
   int32_t getBackInBoundsToleranceY() const;

   /// \brief returns the vertical player sampling offset for camera tracking.
   /// \return y offset in pixels added to the player's position before tracking.
   int32_t getPlayerOffsetY() const;

   /// \brief writes the current camera configuration as json to disk.
   /// \param filename destination json file path.
   void serializeToFile(const std::string& filename = "data/config/camera.json");

   /// \brief reads camera configuration from a json file and applies it.
   /// \param filename source json file path.
   void deserializeFromFile(const std::string& filename = "data/config/camera.json");

   /// \brief serializes all camera parameters into a formatted json string.
   /// \return json text containing the camera configuration object.
   std::string serialize();

   /// \brief parses a json payload and updates camera parameters from it.
   /// \param data json text that contains a CameraSystemConfiguration object.
   void deserialize(const std::string& data);

   // x
   float _camera_velocity_factor_x = 4.0f;
   float _focus_zone_divider = 6.0f;
   float _target_shift_factor = 0.75f;
   int32_t _back_in_bounds_tolerance_x = 10;
   bool _follow_player_orientation = false;

   // y
   float _camera_velocity_factor_y = 3.0f;
   float _panic_line_divider = 2.5f;
   float _view_ratio_y = 1.5f;
   int32_t _back_in_bounds_tolerance_y = 10;
   int32_t _player_offset_y = 0;
   float _panic_acceleration_factor_y = 2.0f;

   // shaking
   bool _camera_shaking_enabled = true;

   static bool sInitialized;
   static CameraSystemConfiguration sInstance;
};
