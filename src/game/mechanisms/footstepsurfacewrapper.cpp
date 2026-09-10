#include "footstepsurfacewrapper.h"

#include "game/level/levelregistry.h"
#include "game/mechanisms/footstepsurface.h"

std::optional<std::string> FootstepSurfaceWrapper::getSurfaceAt(const sf::Vector2f& position_px)
{
   auto level = LevelRegistry::getCurrent();

   if (!level)
   {
      return std::nullopt;
   }

   for (const auto& mechanism : level->getMechanismRegistry().getFootstepSurfaces())
   {
      auto footstep_surface = std::dynamic_pointer_cast<FootstepSurface>(mechanism);

      if (!footstep_surface || !footstep_surface->isEnabled())
      {
         continue;
      }

      const auto& bounding_box_px = footstep_surface->getBoundingBoxPx();

      if (bounding_box_px.has_value() && bounding_box_px->contains(position_px))
      {
         return footstep_surface->getSurface();
      }
   }

   return std::nullopt;
}
