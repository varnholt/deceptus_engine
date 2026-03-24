#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <memory>

/// \brief base interface for effects that animate transitions on a full-screen render texture.
struct ScreenTransitionEffect
{
   std::shared_ptr<sf::RenderTexture> _frame_buffer;

   /// \brief initializes the effect before per-frame processing begins.
   virtual void start();
   /// \brief advances effect state by one frame.
   /// \param dt elapsed frame time since the previous update.
   virtual void update(const sf::Time& /*dt*/);
   /// \brief renders the current transition state to the supplied render texture.
   /// \param window render texture that receives the transition overlay.
   virtual void draw(const std::shared_ptr<sf::RenderTexture>& /*window*/);
   /// \brief virtual destructor for safe polymorphic cleanup.
   virtual ~ScreenTransitionEffect();

   using TransitionCallback = std::function<void()>;
   TransitionCallback _effect_ended;
   bool _done = false;

protected:
   /// \brief marks the effect as finished and invokes the completion callback.
   virtual void done();
};
