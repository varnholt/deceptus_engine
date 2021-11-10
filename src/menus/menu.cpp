#include "menu.h"

#include "game/displaymode.h"
#include "game/gameconfiguration.h"

#include "menuscreenachievements.h"
#include "menuscreenaudio.h"
#include "menuscreencontrols.h"
#include "menuscreencredits.h"
#include "menuscreenfileselect.h"
#include "menuscreengame.h"
#include "menuscreenmain.h"
#include "menuscreennameselect.h"
#include "menuscreenoptions.h"
#include "menuscreenpause.h"
#include "menuscreenvideo.h"

#include "game/gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"

#include <iostream>


std::shared_ptr<Menu> Menu::__instance;


Menu::Menu()
 : _menu_main(std::make_shared<MenuScreenMain>()),
   _menu_file_select(std::make_shared<MenuScreenFileSelect>()),
   _menu_name_select(std::make_shared<MenuScreenNameSelect>()),
   _menu_options(std::make_shared<MenuScreenOptions>()),
   _menu_audio(std::make_shared<MenuScreenAudio>()),
   _menu_video(std::make_shared<MenuScreenVideo>()),
   _menu_controls(std::make_shared<MenuScreenControls>()),
   _menu_game(std::make_shared<MenuScreenGame>()),
   _menu_achievements(std::make_shared<MenuScreenAchievements>()),
   _menu_credits(std::make_shared<MenuScreenCredits>()),
   _menu_pause(std::make_shared<MenuScreenPause>())
{
   _menus.push_back(_menu_main);
   _menus.push_back(_menu_file_select);
   _menus.push_back(_menu_name_select);
   _menus.push_back(_menu_options);
   _menus.push_back(_menu_audio);
   _menus.push_back(_menu_video);
   _menus.push_back(_menu_controls);
   _menus.push_back(_menu_game);
   _menus.push_back(_menu_achievements);
   _menus.push_back(_menu_credits);
   _menus.push_back(_menu_pause);

   for (auto& screen : _menus)
   {
      screen->load();
   }
}


std::shared_ptr<Menu>& Menu::getInstance()
{
   if (!__instance)
   {
      __instance = std::make_shared<Menu>();
      __instance->initialize();
   }

   return __instance;
}


void Menu::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto game_config = GameConfiguration::getInstance();

   auto w = game_config._view_width;
   auto h = game_config._view_height;

   // set up an ortho view with screen dimensions
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));

   window.setView(view);

   if (_current_menu)
   {
      _current_menu->draw(window, states);
   }
}


void Menu::update(const sf::Time& dt)
{
   _current_menu->update(dt);
}


void Menu::show(Menu::MenuType menu)
{
   switch (menu)
   {
      case MenuType::None:
         _current_menu = nullptr;
         break;
      case MenuType::Main:
         _current_menu = _menu_main;
         break;
      case MenuType::Options:
         _current_menu = _menu_options;
         break;
      case MenuType::FileSelect:
         _current_menu = _menu_file_select;
         break;
      case MenuType::NameSelect:
         _current_menu = _menu_name_select;
         break;
      case MenuType::Controls:
         _current_menu = _menu_controls;
         break;
      case MenuType::Video:
         _current_menu = _menu_video;
         break;
      case MenuType::Audio:
         _current_menu = _menu_audio;
         break;
      case MenuType::Game:
         _current_menu = _menu_game;
         break;
      case MenuType::Achievements:
         _current_menu = _menu_achievements;
         break;
      case MenuType::Credits:
         _current_menu = _menu_credits;
         break;
      case MenuType::Pause:
         _current_menu = _menu_pause;
         break;
   }

   _previous_type = _current_type;
   _current_type = menu;

   if (_current_menu)
   {
      _current_menu->showEvent();
      DisplayMode::getInstance().enqueueSet(Display::MainMenu);
   }

   _history.push_back(menu);
   while (_history.size() > 10)
   {
      _history.pop_front();
   }
}


void Menu::hide()
{
   if (_current_menu)
   {
      _current_menu->hideEvent();
   }

   DisplayMode::getInstance().enqueueUnset(Display::MainMenu);

   _current_menu = nullptr;
   _current_type = MenuType::None;
}


void Menu::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (_current_menu == nullptr)
   {
      return;
   }

   _current_menu->keyboardKeyPressed(key);
}


void Menu::keyboardKeyReleased(sf::Keyboard::Key key)
{
   if (_current_menu == nullptr)
   {
      return;
   }

   _current_menu->keyboardKeyReleased(key);
}


void Menu::controllerButtonX()
{
   if (_current_menu == nullptr)
   {
      return;
   }

   _current_menu->controllerButtonX();
}


void Menu::controllerButtonY()
{
   if (_current_menu == nullptr)
   {
      return;
   }

   _current_menu->controllerButtonY();
}


bool Menu::isVisible() const
{
   return (_current_type != MenuType::None);
}


Menu::MenuType Menu::getCurrentType() const
{
   return _current_type;
}


Menu::MenuType Menu::getPreviousType() const
{
   return  _previous_type;
}


const std::deque<Menu::MenuType>& Menu::getHistory() const
{
   return _history;
}


const std::shared_ptr<MenuScreen>& Menu::getMenuScreen(Menu::MenuType type) const
{
    switch (type)
    {
       case MenuType::Main:
          return _menu_main;
       case MenuType::Options:
          return _menu_options;
       case MenuType::FileSelect:
          return _menu_file_select;
       case MenuType::NameSelect:
          return _menu_name_select;
       case MenuType::Controls:
          return _menu_controls;
       case MenuType::Video:
          return _menu_video;
       case MenuType::Audio:
          return _menu_audio;
       case MenuType::Game:
          return _menu_game;
       case MenuType::Achievements:
          return _menu_achievements;
       case MenuType::Credits:
          return _menu_credits;
       case MenuType::Pause:
          return _menu_pause;
        case MenuType::None:
          break;
    }

    return _menu_invalid;
}


void Menu::initialize()
{
   auto gci = GameControllerIntegration::getInstance(0);

   if (gci)
   {
      GameController::ThresholdCallback up;
      up._axis = SDL_CONTROLLER_AXIS_LEFTY;
      up._boundary = GameController::ThresholdCallback::Boundary::Lower;
      up._threshold = -0.3f;
      up._callback = [this](){keyboardKeyPressed(sf::Keyboard::Up);};
      gci->getController()->addAxisThresholdExceedCallback(up);

      GameController::ThresholdCallback down;
      down._axis = SDL_CONTROLLER_AXIS_LEFTY;
      down._callback = [this](){keyboardKeyPressed(sf::Keyboard::Down);};
      down._boundary = GameController::ThresholdCallback::Boundary::Upper;
      down._threshold = 0.3f;
      gci->getController()->addAxisThresholdExceedCallback(down);

      GameController::ThresholdCallback left;
      left._axis = SDL_CONTROLLER_AXIS_LEFTX;
      left._callback = [this](){keyboardKeyPressed(sf::Keyboard::Left);};
      left._boundary = GameController::ThresholdCallback::Boundary::Lower;
      left._threshold = -0.3f;
      gci->getController()->addAxisThresholdExceedCallback(left);

      GameController::ThresholdCallback right;
      right._axis = SDL_CONTROLLER_AXIS_LEFTX;
      right._boundary = GameController::ThresholdCallback::Boundary::Upper;
      right._threshold = 0.3f;
      right._callback = [this](){keyboardKeyPressed(sf::Keyboard::Right);};
      gci->getController()->addAxisThresholdExceedCallback(right);


      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_DPAD_UP,
         [this](){keyboardKeyPressed(sf::Keyboard::Up);}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_DPAD_DOWN,
         [this](){keyboardKeyPressed(sf::Keyboard::Down);}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_DPAD_LEFT,
         [this](){keyboardKeyPressed(sf::Keyboard::Left);}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
         [this](){keyboardKeyPressed(sf::Keyboard::Right);}
      );


      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_A,
         [this](){keyboardKeyPressed(sf::Keyboard::Return);}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_B,
         [this](){keyboardKeyPressed(sf::Keyboard::Escape);}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_X,
         [this](){controllerButtonX();}
      );

      gci->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_Y,
         [this](){controllerButtonY();}
      );
   }
}

