#pragma once

class Tweaks
{
   public:

      static const Tweaks& instance();

      float _bend_down_threshold = 0.6f;

   private:
      Tweaks() = default;
};

