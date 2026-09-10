#ifndef PLAYERFOOTSTEPS_H
#define PLAYERFOOTSTEPS_H

#include <cstdint>
#include <string>

#include <SFML/Graphics.hpp>

/// \brief plays the player's footstep samples, alternating feet and picking the surface below them.
class PlayerFootsteps
{
public:
   /// \brief plays a footstep sample when the player is due for the next step.
   ///
   /// The step rate follows the walking speed, so the samples keep up with the animation.
   /// \param time current player time, the same clock the next step is scheduled on.
   /// \param player_rect_px player rectangle in pixel coordinates, its bottom centre is the contact point.
   /// \param velocity_x horizontal player velocity in meters per second.
   /// \param grounded true while the player has ground contact.
   /// \param in_water true while the player is submerged, which keeps the footsteps silent.
   void update(const sf::Time& time, const sf::FloatRect& player_rect_px, float velocity_x, bool grounded, bool in_water);

private:
   /// \brief picks the surface that applies below the player.
   ///
   /// A footstep surface region wins where one covers the position, otherwise the surface the level
   /// declares applies and, when the level names none, the configured default.
   /// \param foot_position_px point where the player touches the ground, in pixel coordinates.
   /// \return surface identifier to look the samples up with.
   std::string resolveSurface(const sf::Vector2f& foot_position_px) const;

   float _next_footstep_time_s{0.0f};
   int32_t _step_counter{0};  //!< alternates the feet, its lowest bit selects the left or right sample list
};

#endif  // PLAYERFOOTSTEPS_H
