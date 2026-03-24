#ifndef PLAYERDASH_H
#define PLAYERDASH_H

#include <chrono>
#include <cstdint>
#include <functional>
#include "game/constants.h"

class b2Body;

/// \brief executes the stamina-based horizontal dash ability over multiple physics frames.
struct PlayerDash
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   /// \brief per-frame dash input gathered from player state and controls.
   struct DashInput
   {
      Dash _dir;
      bool _wallsliding{false};
      bool _hard_landing{false};
      bool _is_in_water{false};
      bool& _points_to_left;
      b2Body* player_body{nullptr};
   };

   /// \brief processes dash activation and applies dash forces while the dash is active.
   /// \param input dash command, environment state, facing direction reference, and player body.
   void update(const DashInput& input);

   /// \brief cancels the active dash immediately without running remaining dash frames.
   void abort();

   /// \brief reports whether dash force application is still active.
   /// \return true when there are remaining dash frames.
   bool hasMoreFrames() const;

   /// \brief finalizes dash state, restores gravity, and removes dash stamina drain.
   /// \param player_body player body whose gravity scale is restored.
   void reset(b2Body* player_body);

   int32_t _frame_count = 0;
   float _multiplier = 0.0f;
   Dash _direction = Dash::None;
   std::function<void(void)> _reset_dash_callback;
   HighResTimePoint _last_dash_time_point;
};

#endif  // PLAYERDASH_H
