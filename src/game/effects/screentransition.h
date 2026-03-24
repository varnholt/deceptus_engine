#pragma once

#include <SFML/Graphics.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <memory>

struct ScreenTransitionEffect;

/// \brief coordinates one or two transition effects and optional callbacks between their phases.
/// \details each transition runs effect 1 first and can optionally auto-start effect 2 after a delay.
struct ScreenTransition
{
   ScreenTransition() = default;

   /// \brief starts the first effect, wires completion callbacks, and notifies start listeners.
   void startEffect1();
   /// \brief starts the second effect and notifies start listeners.
   void startEffect2();
   /// \brief advances the currently active effect, if any.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);
   /// \brief draws the currently active effect to the provided render texture.
   /// \param window render texture used for transition rendering.
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
   /// \brief handles completion of effect 1, runs end callbacks, and optionally schedules effect 2.
   void effect1Done();
   /// \brief handles completion of effect 2, clears active effect state, and runs end callbacks.
   void effect2Done();

   std::shared_ptr<ScreenTransitionEffect> _active_effect;
};

/// \brief owns a queue of screen transitions and processes only the front transition.
struct ScreenTransitionHandler
{
   /// \brief appends a transition to the queue without starting it automatically.
   /// \param transition transition object transferred into the handler queue.
   void push(std::unique_ptr<ScreenTransition>);
   /// \brief removes the front transition when one is active.
   void pop();
   /// \brief clears all queued transitions.
   void clear();
   /// \brief starts effect 2 on the front transition if the queue is not empty.
   void startEffect2();
   /// \brief reports whether at least one transition is queued.
   /// \return true if the handler has an active front transition.
   bool active() const;

   std::deque<std::unique_ptr<ScreenTransition>> _transitions;

   /// \brief draws the front transition if one is active.
   /// \param window render texture used by the transition effect.
   void draw(const std::shared_ptr<sf::RenderTexture>& window);
   /// \brief advances the front transition if one is active.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief retrieves the global transition handler singleton.
   /// \return reference to the shared transition handler instance.
   static ScreenTransitionHandler& getInstance();
};
