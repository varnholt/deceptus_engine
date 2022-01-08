#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <memory>


struct ScreenTransitionEffect
{
   std::shared_ptr<sf::RenderTexture> _frame_buffer;

   virtual void start();
   virtual void update(const sf::Time& /*dt*/);
   virtual void draw(const std::shared_ptr<sf::RenderTexture>& /*window*/);
   virtual ~ScreenTransitionEffect();

   using TransitionCallback = std::function<void()>;
   TransitionCallback _effect_ended;
   bool _done = false;

protected:
   virtual void done();
};

