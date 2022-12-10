#include "ingamemenu.h"

#include "displaymode.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gamecontrollerintegration.h"
#include "gamestate.h"

#include <iostream>

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}

void InGameMenu::setAudioCallback(const AudioCallback&)
{
}

void InGameMenu::initializeController()
{
   auto& gji = GameControllerIntegration::getInstance();

   gji.addDeviceAddedCallback(
      [this](int32_t /*id*/)
      {
         auto& gji = GameControllerIntegration::getInstance();
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_Y, [this]() { open(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, [this]() { close(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, [this]() { close(); });
      }
   );
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
   const auto& gci = GameControllerIntegration::getInstance();

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
InGameMenu::InGameMenu()
{
   _audio_callback = [this](InGameMenuAudio::SoundEffect effect) { _audio.play(effect); };

   _menu_archives = std::make_shared<InGameMenuArchives>();
   _menu_inventory = std::make_shared<InGameMenuInventory>();
   _menu_map = std::make_shared<IngameMenuMap>();

   _submenu_type_map[static_cast<uint8_t>(SubMenu::Map)] = _menu_map;
   _submenu_type_map[static_cast<uint8_t>(SubMenu::Inventory)] = _menu_inventory;
   _submenu_type_map[static_cast<uint8_t>(SubMenu::Archives)] = _menu_archives;

   _submenu_type_names[static_cast<uint8_t>(SubMenu::Map)] = "map";
   _submenu_type_names[static_cast<uint8_t>(SubMenu::Inventory)] = "inventory";
   _submenu_type_names[static_cast<uint8_t>(SubMenu::Archives)] = "archives";

   _submenu_selection = {SubMenu::Map, SubMenu::Inventory, SubMenu::Archives};

   // rotate until we have selected the default
   while (_submenu_selection[0] != SubMenu::Inventory)
   {
      rotateRight();
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (_previous_submenu.has_value())
   {
      // draw previous menu only if it is animated
      auto menu = _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())];
      if (menu->getAnimation().has_value())
      {
         menu->draw(window, states);
      }
   }

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->draw(window, states);
}

//----------------------------------------------------------------------------------------------------------------------
void InGameMenu::updateController()
{
   auto& gji = GameControllerIntegration::getInstance();
   if (gji.isControllerConnected())
   {
      setJoystickInfo(gji.getController()->getInfo());
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::update(const sf::Time& dt)
{
   updateController();
   updateControllerActions();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->update(dt);

   if (_previous_submenu.has_value())
   {
      _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->update(dt);
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::processEvent(const sf::Event& event)
{
   switch (event.key.code)
   {
      case sf::Keyboard::Left:
      {
         left();
         break;
      }
      case sf::Keyboard::Right:
      {
         right();
         break;
      }
      case sf::Keyboard::Up:
      {
         up();
         break;
      }
      case sf::Keyboard::Down:
      {
         down();
         break;
      }
      case sf::Keyboard::LShift:
      case sf::Keyboard::Q:
      {
         goToLeftSubMenu();
         break;
      }
      case sf::Keyboard::RShift:
      case sf::Keyboard::W:
      {
         goToRightSubMenu();
         break;
      }
      case sf::Keyboard::Return:
      {
         close();
         break;
      }
      default:
      {
         break;
      }
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::open()
{
   if (GameState::getInstance().getMode() != ExecutionMode::Running)
   {
      return;
   }

   // disallow in-game menu during screen transitions
   if (DisplayMode::getInstance().isSet(Display::ScreenTransition))
   {
      return;
   }

   GameState::getInstance().enqueuePause();
   DisplayMode::getInstance().enqueueSet(Display::IngameMenu);

   _audio_callback(InGameMenuAudio::SoundEffect::MenuOpen);
   show();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::close()
{
   if (!DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      return;
   }

   _audio_callback(InGameMenuAudio::SoundEffect::MenuClose);
   hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::left()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->left();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::right()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->right();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::up()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->up();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::down()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->down();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::show()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->show();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::hide()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::rotateRight()
{
   std::rotate(_submenu_selection.begin(), _submenu_selection.begin() + 1, _submenu_selection.end());

   _selected_submenu = _submenu_selection[0];
   _previous_submenu = _submenu_selection[2];
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::goToRightSubMenu()
{
   rotateRight();

   // debug();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->moveInFromRight();
   _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->moveOutToLeft();

   _audio_callback(InGameMenuAudio::SoundEffect::MenuNext);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::rotateLeft()
{
   std::rotate(_submenu_selection.rbegin(), _submenu_selection.rbegin() + 1, _submenu_selection.rend());

   _selected_submenu = _submenu_selection[0];
   _previous_submenu = _submenu_selection[1];
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::goToLeftSubMenu()
{
   rotateLeft();

   // debug();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->moveInFromLeft();
   _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->moveOutToRight();

   _audio_callback(InGameMenuAudio::SoundEffect::MenuNext);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::debug()
{
   const auto selected = _submenu_type_names[static_cast<uint8_t>(_selected_submenu)];
   const auto previous = (_previous_submenu.has_value() ? _submenu_type_names[static_cast<uint8_t>(_previous_submenu.value())] : "n/a");

   std::cout << "selected: " << selected << ", previous: " << previous << std::endl;
}
