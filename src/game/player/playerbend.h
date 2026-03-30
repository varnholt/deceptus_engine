#ifndef PLAYERBEND_H
#define PLAYERBEND_H

#include <chrono>

/// \brief stores crouch and bend transition state used by movement and animation.
struct PlayerBend
{
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   bool _bending_down = false;
   bool _was_bending_down = false;

   bool _crouching = false;
   bool _was_crouching = false;

   HighResTimePoint _timepoint_bend_down_start;
   HighResTimePoint _timepoint_bend_down_end;

   /// \brief clears bend-down state flags, typically when another move interrupts crouching.
   void resetBendingDown()
   {
      _bending_down = false;
      _was_bending_down = false;
   }

   /// \brief sets bend-down state while preserving the previous frame value.
   /// \param bending_down true when the player is currently bending down.
   void setBendingDownEnabled(bool bending_down)
   {
      _was_bending_down = _bending_down;
      _bending_down = bending_down;
   }

   /// \brief sets crouch-enabled state while preserving the previous frame value.
   /// \param crouching true when crouch movement restrictions are active.
   void setCrouchingEnabled(bool crouching)
   {
      _was_crouching = _crouching;
      _crouching = crouching;
   }

   /// \brief reports whether crouch logic is currently active.
   /// \return true when bend-down movement restrictions apply.
   bool isCrouching() const
   {
      return _bending_down;
   }
};

#endif  // PLAYERBEND_H
