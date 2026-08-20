#include "fixedtimestep.h"

#include "game/physics/physicsconfiguration.h"
#include "game/physics/renderinterpolation.h"

int32_t FixedTimeStep::consumeSteps(const sf::Time& dt)
{
   const auto step_duration_s = getStepDurationInS();
   if (step_duration_s <= 0.0f)
   {
      return 1;
   }

   _accumulated_s += dt.asSeconds();

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
   return PhysicsConfiguration::getInstance()._time_step;
}

sf::Time FixedTimeStep::getStepDuration() const
{
   return sf::seconds(getStepDurationInS());
}

void FixedTimeStep::reset()
{
   _accumulated_s = 0.0f;
}
