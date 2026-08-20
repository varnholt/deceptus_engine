#pragma once

#include <cstdint>

#include <SFML/System/Time.hpp>

/// \brief turns a variable frame time into a whole number of fixed simulation steps.
///
/// The simulation is written against one assumption: it is updated exactly once per physics step,
/// with a delta of PhysicsConfiguration::_time_step. Driving it straight from the frame time breaks
/// that - the world advanced one step per frame whatever the frame rate, so it ran at double speed
/// at 120 fps and in slow motion whenever the frame rate dropped below 60.
///
/// This restores the assumption instead of spreading frame rate compensation across the code that
/// relies on it: leftover frame time is banked here, and the caller runs the whole simulation update
/// as many whole steps as the bank affords. A frame that is faster than the step runs none of them
/// and simply draws the same state again.
class FixedTimeStep
{
public:
   /// \brief adds a frame's worth of time and reports how many steps it pays for.
   /// \param dt time elapsed since the previous frame.
   /// \return number of simulation steps to run this frame, never more than the catch-up limit.
   /// \note the time behind the limit is discarded rather than banked. Keeping it would leave the
   ///       simulation permanently in catch-up after a single long frame - a level load, a breakpoint
   ///       - and each frame of catch-up costs more time, so it never gets out.
   int32_t consumeSteps(const sf::Time& dt);

   /// \brief duration of one simulation step, in seconds.
   /// \return the step the physics world is advanced by, from PhysicsConfiguration.
   float getStepDurationInS() const;

   /// \brief duration of one simulation step, as the delta handed to the simulation update.
   sf::Time getStepDuration() const;

   /// \brief drops the banked time.
   /// \note call this whenever real time has passed that the simulation must not make up for, i.e.
   ///       after loading a level.
   void reset();

private:
   //!< frame time banked but not yet spent on a step
   float _accumulated_s{0.0f};

   //!< most steps one frame may run. below the resulting frame rate the game slows down rather than
   //!< spending ever more of each frame catching up
   static constexpr int32_t max_steps_per_frame = 5;
};
