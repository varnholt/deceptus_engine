#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <functional>
#include <memory>


struct ScreenTransitionEffect;


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
   std::unique_ptr<ScreenTransition> _transition;

   static ScreenTransitionHandler& getInstance();
};

