#ifndef PLAYERDASH_H
#define PLAYERDASH_H

#include <chrono>
#include <cstdint>
#include <functional>
#include "game/constants.h"

class b2Body;

struct PlayerDash
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   struct DashInput
   {
      Dash _dir;
      bool _wallsliding{false};
      bool _hard_landing{false};
      bool _is_in_water{false};
      bool& _points_to_left;
      b2Body* player_body{nullptr};
   };

   void update(const DashInput& input);
   void abort();
   bool hasMoreFrames() const;
   void reset(b2Body* player_body);

   int32_t _frame_count = 0;
   float _multiplier = 0.0f;
   Dash _direction = Dash::None;
   std::function<void(void)> _reset_dash_callback;
   HighResTimePoint _last_dash_time_point;
};

#endif // PLAYERDASH_H
