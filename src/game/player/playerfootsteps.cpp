#include "playerfootsteps.h"

#include "game/audio/audio.h"
#include "game/audio/footstepsurfaces.h"
#include "game/level/levelregistry.h"

#include <cmath>

void PlayerFootsteps::update(const sf::Time& time, const sf::FloatRect& player_rect_px, float velocity_x, bool grounded, bool in_water)
{
   if (!grounded || in_water)
   {
      return;
   }

   auto velocity = fabs(velocity_x);
   if (velocity <= 0.1f)
   {
      return;
   }

   if (velocity < 3.0f)
   {
      velocity = 3.0f;
   }

   if (time.asSeconds() <= _next_footstep_time_s)
   {
      return;
   }

   // the surface is looked up where the player touches the ground, not at the body centre,
   // so a footstep surface rectangle does not have to be player height
   const sf::Vector2f foot_position_px{
      player_rect_px.position.x + player_rect_px.size.x * 0.5f, player_rect_px.position.y + player_rect_px.size.y
   };

   const auto& surface = LevelRegistry::getCurrent()->getFootstepSurface(foot_position_px);
   const auto definition = FootstepSurfaces::findDefinition(surface);

   if (definition.has_value())
   {
      const auto sample = definition->pickSample((_step_counter++ & 1) != 0);

      if (sample.has_value())
      {
         Audio::getInstance().playSample({sample.value(), definition->_volume});
      }
   }

   _next_footstep_time_s = time.asSeconds() + 1.0f / velocity;
}
