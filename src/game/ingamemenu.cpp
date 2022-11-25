#include "ingamemenu.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gamecontrollerintegration.h"

#include <iostream>


//---------------------------------------------------------------------------------------------------------------------
GameControllerInfo InGameMenu::getJoystickInfo() const
{
   return _joystick_info;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}


//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   _inventory.draw(window, states);
}

//---------------------------------------------------------------------------------------------------------------------
bool InGameMenu::isControllerActionSkipped() const
{
   auto skipped = false;
   auto now = GlobalClock::getInstance().getElapsedTimeInS();

   if (now - _joystick_update_time < 0.3f)
   {
      skipped = true;
   }

   return skipped;
}


//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::updateControllerActions()
{
   auto& gci = GameControllerIntegration::getInstance();

   if (!gci.isControllerConnected())
   {
      return;
   }

   const auto axis_values = _joystick_info.getAxisValues();
   const auto axis_left_x = gci.getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   const auto hat_values = _joystick_info.getHatValues().at(0);
   const auto dpad_left_pressed = hat_values & SDL_HAT_LEFT;
   const auto dpad_right_pressed = hat_values & SDL_HAT_RIGHT;
   auto xl = axis_values[axis_left_x] / 32767.0f;

   if (dpad_left_pressed)
   {
      xl = -1.0f;
   }
   else if (dpad_right_pressed)
   {
      xl = 1.0f;
   }

   if (fabs(xl) > 0.3f)
   {
      if (xl < 0.0f)
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            left();
         }
      }
      else
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            right();
         }
      }
   }
   else
   {
      _joystick_update_time = 0.0f;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::update(const sf::Time& dt)
{
   updateControllerActions();

   _inventory.update(dt);
}


//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::left()
{
   _inventory.left();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::right()
{
   _inventory.right();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::show()
{
   _inventory.show();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::hide()
{
   _inventory.hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::confirm()
{
   hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::cancel()
{
   hide();
}
