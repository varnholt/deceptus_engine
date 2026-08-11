#pragma once

#include <SFML/Graphics.hpp>

#include <memory>

class GrabRope;

/// \brief finds grab ropes for the player side without exposing the level to it.
namespace GrabRopeWrapper
{
/// \brief returns the grab rope overlapping a rectangle.
/// \param rect_px rectangle to test against the rope bounding boxes, in pixels.
/// \return the first overlapping grab rope, or nullptr when none overlaps.
std::shared_ptr<GrabRope> getGrabRopeAt(const sf::FloatRect& rect_px);
}  // namespace GrabRopeWrapper
