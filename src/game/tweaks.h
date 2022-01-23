#pragma once

#include "framework/tools/jsonconfiguration.h"

class Tweaks : public JsonConfiguration
{
   public:

      static const Tweaks& instance();

      float _bend_down_threshold = 0.6f;
      float _cpan_tolerance_x = 0.2f;
      float _cpan_tolerance_y = 0.2f;
      float _cpan_max_distance_px = 100.0f;
      float _cpan_look_speed_x = 4.0f;
      float _cpan_look_speed_y = 3.0f;
      float _cpan_snap_back_factor = 0.85f;
      float _enter_portal_threshold = -0.6f;

   private:

      Tweaks() = default;
      std::string serialize() override;
      void deserialize(const std::string& data) override;
      bool initialized = false;
};

