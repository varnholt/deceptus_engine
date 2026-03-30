#pragma once

#include "framework/tools/jsonconfiguration.h"

/// \brief stores gameplay tuning values loaded from data/config/tweaks.json.
class Tweaks : public JsonConfiguration
{
public:
   /// \brief returns the global tweaks object and lazily loads it from disk.
   /// \return singleton tweaks instance used across gameplay systems.
   static Tweaks& instance();

   float _bend_down_threshold = 0.6f;
   float _cpan_tolerance_x = 0.2f;
   float _cpan_tolerance_y = 0.2f;
   float _cpan_max_distance_px = 100.0f;
   float _cpan_look_speed_x = 4.0f;
   float _cpan_look_speed_y = 3.0f;
   float _cpan_snap_back_factor = 0.85f;
   bool _cpan_unlimited = false;
   float _enter_portal_threshold = -0.6f;
   bool _player_light_enabled = true;
   uint8_t _player_light_alpha = 10;
   uint8_t _player_stencil_alpha = 40;

private:
   /// \brief constructs the tweaks container with built-in fallback values.
   Tweaks() = default;

   /// \brief serializes tweak fields into json text for persistence.
   /// \return json string containing the Tweaks object.
   std::string serialize() override;

   /// \brief parses tweak json and updates known fields when present.
   /// \param data json payload containing a Tweaks object.
   void deserialize(const std::string& data) override;
   bool initialized = false;
};
