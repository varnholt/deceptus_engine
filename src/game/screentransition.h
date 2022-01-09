#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <memory>


struct ScreenTransitionEffect;

/*! \brief A ScreenTransition is an animated effect transitioning from one screen to aother.
 *         This could be something as simple as 'fade out', 'fade in'.
 *
 *  Each transition can either have one or two defined effects that may have a delay between them.
 *  The transitions are linked using a simple callback, TransitionCallback.
 */
struct ScreenTransition
{
   ScreenTransition() = default;

   void startEffect1();
   void startEffect2();
   void update(const sf::Time& dt);
   void draw(const std::shared_ptr<sf::RenderTexture>& window);

   std::shared_ptr<ScreenTransitionEffect> _effect_1;
   std::shared_ptr<ScreenTransitionEffect> _effect_2;
   std::chrono::milliseconds _delay_between_effects_ms;

   using TransitionCallback = std::function<void()>;

   std::vector<TransitionCallback> _callbacks_effect_1_started;
   std::vector<TransitionCallback> _callbacks_effect_1_ended;
   std::vector<TransitionCallback> _callbacks_effect_2_started;
   std::vector<TransitionCallback> _callbacks_effect_2_ended;
   bool _autostart_effect_2 = true;

private:
   void effect1Done();
   void effect2Done();

   std::shared_ptr<ScreenTransitionEffect> _active_effect;
};


struct ScreenTransitionHandler
{
   void push(std::unique_ptr<ScreenTransition>);
   void pop();
   void clear();
   void startEffect2();
   bool active() const;

   std::deque<std::unique_ptr<ScreenTransition>> _transitions;

   void draw(const std::shared_ptr<sf::RenderTexture>& window);
   void update(const sf::Time& dt);

   static ScreenTransitionHandler& getInstance();
};

