#ifndef FOOTSTEPSURFACEWRAPPER_H
#define FOOTSTEPSURFACEWRAPPER_H

#include <SFML/System/Vector2.hpp>
#include <optional>
#include <string>

/// \brief finds footstep surface regions for the player side without exposing the level to it.
namespace FootstepSurfaceWrapper
{
/// \brief returns the surface of the region covering a position.
/// \param position_px position to test against the region bounding boxes, in pixels.
/// \return surface of the first enabled region covering the position, or std::nullopt when none does.
std::optional<std::string> getSurfaceAt(const sf::Vector2f& position_px);
};  // namespace FootstepSurfaceWrapper

#endif  // FOOTSTEPSURFACEWRAPPER_H
