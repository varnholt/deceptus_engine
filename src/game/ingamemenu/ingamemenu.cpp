#include "ingamemenu.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/state/displaymode.h"
#include "game/state/gamestate.h"

#include <iostream>

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
         const auto& gji = GameControllerIntegration::getInstance();
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_BACK, [this]() { open(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_BACK, [this]() { close(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, [this]() { close(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, [this]() { close(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, [this]() { goToLeftSubMenu(); });
         gji.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, [this]() { goToRightSubMenu(); });
      }
   );
}

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

void InGameMenu::updateControllerActions()
{
   const auto& gci = GameControllerIntegration::getInstance();

   if (!gci.isControllerConnected())
   {
      return;
   }

   const auto axis_values = _joystick_info.getAxisValues();
   const auto axis_left_x = gci.getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   const auto axis_left_y = gci.getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTY);
   const auto hat_values = _joystick_info.getHatValues().at(0);

   auto xl = axis_values[axis_left_x] / 32767.0f;
   auto yl = axis_values[axis_left_y] / 32767.0f;

   if (static_cast<bool>(hat_values & SDL_HAT_LEFT))
   {
      xl = -1.0f;
   }
   else if (static_cast<bool>(hat_values & SDL_HAT_RIGHT))
   {
      xl = 1.0f;
   }

   if (static_cast<bool>(hat_values & SDL_HAT_UP))
   {
      yl = -1.0f;
   }
   else if (static_cast<bool>(hat_values & SDL_HAT_DOWN))
   {
      yl = 1.0f;
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
   else if (fabs(yl) > 0.3f)
   {
      if (yl < 0.0f)
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            up();
         }
      }
      else
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            down();
         }
      }
   }
   else
   {
      _joystick_update_time = 0.0f;
   }
}

InGameMenu::InGameMenu()
{
   initializeController();

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

void InGameMenu::updateController()
{
   const auto& gji = GameControllerIntegration::getInstance();
   if (gji.isControllerConnected())
   {
      setJoystickInfo(gji.getController()->getInfo());
   }
}

void InGameMenu::update(const sf::Time& delta_time)
{
   updateController();
   updateControllerActions();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->update(delta_time);

   if (_previous_submenu.has_value())
   {
      _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->update(delta_time);
   }
}

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
      case sf::Keyboard::Tab:
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

void InGameMenu::close()
{
   if (!DisplayMode::getInstance().isSet(Display::IngameMenu))
   {
      return;
   }

   _audio_callback(InGameMenuAudio::SoundEffect::MenuClose);
   hide();
}

void InGameMenu::left()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->left();
}

void InGameMenu::right()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->right();
}

void InGameMenu::up()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->up();
}

void InGameMenu::down()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->down();
}

void InGameMenu::show()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->show();
}

void InGameMenu::hide()
{
   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->hide();
}

void InGameMenu::rotateRight()
{
   std::rotate(_submenu_selection.begin(), _submenu_selection.begin() + 1, _submenu_selection.end());

   _selected_submenu = _submenu_selection[0];
   _previous_submenu = _submenu_selection[2];
}

void InGameMenu::goToRightSubMenu()
{
   rotateRight();

   // debug();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->moveInFromRight();
   _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->moveOutToLeft();

   _audio_callback(InGameMenuAudio::SoundEffect::MenuNext);
}

void InGameMenu::rotateLeft()
{
   std::rotate(_submenu_selection.rbegin(), _submenu_selection.rbegin() + 1, _submenu_selection.rend());

   _selected_submenu = _submenu_selection[0];
   _previous_submenu = _submenu_selection[1];
}

void InGameMenu::goToLeftSubMenu()
{
   rotateLeft();

   // debug();

   _submenu_type_map[static_cast<uint8_t>(_selected_submenu)]->moveInFromLeft();
   _submenu_type_map[static_cast<uint8_t>(_previous_submenu.value())]->moveOutToRight();

   _audio_callback(InGameMenuAudio::SoundEffect::MenuNext);
}

void InGameMenu::debug()
{
   const auto selected = _submenu_type_names[static_cast<uint8_t>(_selected_submenu)];
   const auto previous = (_previous_submenu.has_value() ? _submenu_type_names[static_cast<uint8_t>(_previous_submenu.value())] : "n/a");

   std::cout << "selected: " << selected << ", previous: " << previous << std::endl;
}
