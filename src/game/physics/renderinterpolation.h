#pragma once

#include <SFML/System/Vector2.hpp>

/// \brief positions the frame being drawn between the last two simulation states.
///
/// The simulation advances in whole steps of PhysicsConfiguration::_time_step, see FixedTimeStep.
/// That fixes the speed of the world but it also means a frame drawn faster than the step rate shows
/// a state it has already shown, and the step lands on a frame boundary rather than on an even
/// interval - so a run at 200 fps stutters even though the motion underneath is uniform.
///
/// The frame therefore draws where things are *between* two steps: the caller keeps the position a
/// body had before the last step alongside its current one, and asks for the point in between.
///
/// The interpolated position is rounded to whole pixels. The game is pixel art rendered at an
/// integer scale (see GameConfiguration::computeViewScale), and a sprite at a fractional position
/// samples its texels across a screen pixel boundary - the same smearing an unfloored view scale
/// produced. Rounding keeps every sprite exact and still yields one distinct position per pixel of
/// travel, which is what makes the motion read as smooth.
namespace RenderInterpolation
{

/// \brief publishes how far the frame about to be drawn sits past the last simulation step.
/// \param alpha 0 at the step just taken, approaching 1 at the next one.
/// \note there is one simulation clock in the process, so this is held centrally rather than
///       threaded through every draw signature.
void setAlpha(float alpha);

/// \brief how far the frame being drawn sits past the last simulation step, 0..1.
float getAlpha();

/// \brief the position to draw at, between two simulation states.
/// \param previous_px where it was before the last step.
/// \param current_px where it is after the last step.
/// \return the point in between, rounded to whole pixels.
sf::Vector2f positionPx(const sf::Vector2f& previous_px, const sf::Vector2f& current_px);

/// \brief the value to draw at, between two simulation states, for a single axis.
/// \param previous_px where it was before the last step.
/// \param current_px where it is after the last step.
/// \return the value in between, rounded to a whole pixel.
float valuePx(float previous_px, float current_px);

}  // namespace RenderInterpolation
