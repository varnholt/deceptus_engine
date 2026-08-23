#include "fixedtimestep.h"

#include <cmath>

#include "game/physics/renderinterpolation.h"

namespace
{

//!< how far a frame may sit from a whole number of steps and still be taken to be that many.
//!< 0.15 of a step is 2.5 ms, which covers the spread a vsynced frame arrives with
constexpr auto snap_tolerance_in_steps = 0.15f;

/// \brief rounds a frame time that is nearly a whole number of steps to exactly that many.
///
/// A vsynced frame is meant to be one step and measures 15.7 to 18.2 ms against a step of 16.667 ms,
/// so banking the difference makes the bank wander across a step boundary once or twice a second: one
/// frame runs no step and draws the state it just drew, the next runs two and moves everything twice
/// as far. Nothing is wrong with the average speed, which is why this reads as the scrolling being
/// uneven rather than as the game running fast or slow.
///
/// Snapping spends the frame on whole steps instead, so a run at the step rate takes exactly one step
/// per frame however the frame times scatter. A frame that is not near a whole number of steps - half
/// a step at 120 Hz, a third at 180 - is left alone and banked as before, and so is a frame too short
/// to pay for one.
///
/// The tolerance is what decides which refresh rates count as "the step rate": 0.15 of a step spans
/// 52 to 71 Hz, which takes in the 59.94 and 60 Hz a 60 Hz panel actually reports and leaves 50, 72
/// and 75 Hz outside. A display inside that span but not at 60 - 65 Hz, say - is deliberately run at
/// its own rate rather than at 60, i.e. 8% fast and smooth rather than at the right speed with a
/// visible hitch every dozen frames. Nothing here runs at real time anyway, see the class note.
float snapToWholeSteps(float frame_duration_s, float step_duration_s)
{
   const auto steps_in_frame = frame_duration_s / step_duration_s;
   const auto whole_steps = std::round(steps_in_frame);

   if (whole_steps < 1.0f || std::fabs(steps_in_frame - whole_steps) > snap_tolerance_in_steps)
   {
      return frame_duration_s;
   }

   return whole_steps * step_duration_s;
}

}  // namespace

int32_t FixedTimeStep::consumeSteps(const sf::Time& dt)
{
   const auto step_duration_s = getStepDurationInS();
   if (step_duration_s <= 0.0f)
   {
      return 1;
   }

   _accumulated_s += snapToWholeSteps(dt.asSeconds(), step_duration_s);

   auto step_count = static_cast<int32_t>(_accumulated_s / step_duration_s);

   if (step_count > max_steps_per_frame)
   {
      step_count = max_steps_per_frame;
      _accumulated_s = 0.0f;
   }
   else if (step_count > 0)
   {
      _accumulated_s -= static_cast<float>(step_count) * step_duration_s;
   }

   // what is left over is how far the frame about to be drawn sits past the step just taken, which
   // is what the draw path interpolates by. Without it a frame faster than the step rate would draw
   // a state it has already drawn, and the step would land on a frame boundary rather than on an
   // even interval - which is what makes an uncapped run stutter
   RenderInterpolation::setAlpha(_accumulated_s / step_duration_s);

   return step_count;
}

float FixedTimeStep::getStepDurationInS() const
{
   return 1.0f / steps_per_second;
}

sf::Time FixedTimeStep::getStepDuration() const
{
   return sf::seconds(getStepDurationInS());
}

void FixedTimeStep::reset()
{
   _accumulated_s = 0.0f;
}
