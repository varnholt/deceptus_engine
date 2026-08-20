#include "fixedtimestep.h"

#include "game/physics/physicsconfiguration.h"

int32_t FixedTimeStep::consumeSteps(const sf::Time& dt)
{
   const auto step_duration_s = getStepDurationInS();
   if (step_duration_s <= 0.0f)
   {
      return 1;
   }

   _accumulated_s += dt.asSeconds();

   auto step_count = static_cast<int32_t>(_accumulated_s / step_duration_s);
   if (step_count <= 0)
   {
      return 0;
   }

   if (step_count > max_steps_per_frame)
   {
      step_count = max_steps_per_frame;
      _accumulated_s = 0.0f;
      return step_count;
   }

   _accumulated_s -= static_cast<float>(step_count) * step_duration_s;
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
