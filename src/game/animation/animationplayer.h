#pragma once

#include "animation.h"

#include <vector>

#include <SFML/Graphics.hpp>

/// \brief singleton helper that updates and draws transient animations and removes finished ones.
class AnimationPlayer
{
public:
   /// \brief constructs an empty animation player.
   AnimationPlayer() = default;

   /// \brief appends one animation to the managed playback list.
   /// \param animation animation instance to update and draw.
   void add(const std::shared_ptr<Animation>& animation);

   /// \brief appends multiple animations to the managed playback list.
   /// \param animations animation instances to update and draw.
   void add(const std::vector<std::shared_ptr<Animation>>& animations);

   /// \brief updates all managed animations and removes entries that are paused.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief draws all managed animations to the provided render target.
   /// \param target render target that receives all active animations.
   /// \param states render states applied while drawing (used in WASM to carry the level view).
   void draw(sf::RenderTarget& target, const sf::RenderStates& states = {});

   /// \brief returns the global animation-player singleton instance.
   /// \return reference to the shared animation player.
   static AnimationPlayer& getInstance();

private:
   std::vector<std::shared_ptr<Animation>> _animations;
};
