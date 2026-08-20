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
/// The interpolated position is deliberately NOT rounded to whole pixels. Rounding it looks like the
/// right thing for pixel art, and it is not: the camera and the things it follows would each be
/// rounded on their own, so a sprite's screen position - the difference between the two - flips by a
/// pixel whenever one rounds up and the other does not. That happens once per distinct interpolation
/// fraction, so the higher the frame rate the more often the whole scene shivers. Sprites sat on
/// fractional positions before any of this existed; what needed to be exact was the view scale, see
/// GameConfiguration::computeViewScale.
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

/// \brief remembers where something was across simulation steps, so it can be drawn in between.
///
/// Every mechanism that moves shares the same shape: a physics body advances once per simulation
/// step, and a sprite is placed at its position plus an offset. Rather than each of them keeping its
/// own pair of positions, they hold one of these: step() during update, getPositionPx() where the
/// sprite is placed.
class InterpolatedPosition
{
public:
   /// \brief records the position this simulation step moved to.
   /// \param x_px position along x, in pixels.
   /// \param y_px position along y, in pixels.
   void step(float x_px, float y_px);

   /// \brief where to place the sprite for the frame about to be drawn.
   /// \return the position between the last two steps, rounded to whole pixels.
   sf::Vector2f getPositionPx() const;

private:
   sf::Vector2f _previous_px;
   sf::Vector2f _current_px;
};
