#pragma once

class Tweaks
{
   public:

      static const Tweaks& instance();

      float _bend_down_threshold = 0.6f;
      float _cpan_tolerance_x = 0.2f;
      float _cpan_tolerance_y = 0.2f;

   private:
      Tweaks() = default;
};

