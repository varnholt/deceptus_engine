#ifndef PLAYERBEND_H
#define PLAYERBEND_H

#include <chrono>

struct PlayerBend
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   bool _bending_down = false;
   bool _was_bending_down = false;

   bool _crouching = false;
   bool _was_crouching = false;

   HighResTimePoint _timepoint_bend_down_start;
   HighResTimePoint _timepoint_bend_down_end;

   void resetBendingDown()
   {
      _bending_down = false;
      _was_bending_down = false;
   }

   void setBendingDownEnabled(bool bending_down)
   {
      _was_bending_down = _bending_down;
      _bending_down = bending_down;
   }

   void setCrouchingEnabled(bool crouching)
   {
      _was_crouching = _crouching;
      _crouching = crouching;
   }

   bool isCrouching() const
   {
      return _bending_down;
   }
};

#endif  // PLAYERBEND_H
