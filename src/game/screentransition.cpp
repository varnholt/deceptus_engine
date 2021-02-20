#include "screentransition.h"

#include "framework/tools/timer.h"
#include "screentransitioneffect.h"


void ScreenTransition::startEffect1()
{
   _active_effect = _effect_1;

   _effect_1->_effect_ended = [&](){effect1Done();};
   _effect_2->_effect_ended = [&](){effect2Done();};

   _effect_1->start();

   // tell everyone that effect 1 started
   for (auto& cb : _callbacks_effect_1_started)
   {
      cb();
   }
}


void ScreenTransition::update(const sf::Time& dt)
{
   if (_active_effect)
   {
      _active_effect->update(dt);
   }
}


void ScreenTransition::draw(const std::shared_ptr<sf::RenderTexture>& window)
{
   if (_active_effect)
   {
      _active_effect->draw(window);
   }
}


void ScreenTransition::startEffect2()
{
   _effect_2->start();

   // tell everyone that effect 2 started
   for (auto& cb : _callbacks_effect_2_started)
   {
      cb();
   }
}

void ScreenTransition::effect1Done()
{
   // tell everyone that effect 1 is done
   for (auto& cb : _callbacks_effect_1_ended)
   {
      cb();
   }

   _active_effect = _effect_2;

   if (_delay_between_effects_ms.count() == 0)
   {
      Timer::add(_delay_between_effects_ms, [this](){startEffect2();});
   }
   else
   {
      startEffect2();
   }
}


void ScreenTransition::effect2Done()
{
   _active_effect = nullptr;

   // tell everyone that effect 2 is done
   for (auto& cb : _callbacks_effect_2_ended)
   {
      cb();
   }
}


ScreenTransitionHandler& ScreenTransitionHandler::getInstance()
{
   static ScreenTransitionHandler _instance;
   return _instance;
}
