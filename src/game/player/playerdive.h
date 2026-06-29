#ifndef PLAYERDIVE_H
#define PLAYERDIVE_H

#include <SFML/Graphics.hpp>
#include <chrono>

/// \brief tracks how long the player has stayed underwater.
class PlayerDive
{
public:
   /// \brief constructs dive tracking with zero elapsed underwater time.
   PlayerDive() = default;

   /// \brief accumulates underwater time and triggers a warning message after very long dives.
   /// \param dt elapsed frame time.
   /// \param in_water true when the player is currently inside water.
   void update(const sf::Time& dt, bool in_water);

   using HighResDuration = std::chrono::high_resolution_clock::duration;

   HighResDuration _dive_duration;
};

#endif  // PLAYERDIVE_H
