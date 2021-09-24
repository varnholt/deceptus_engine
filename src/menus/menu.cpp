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


std::shared_ptr<Menu> Menu::sInstance;


Menu::Menu()
{
   mMenuMain = std::make_shared<MenuScreenMain>();
   mMenuFileSelect = std::make_shared<MenuScreenFileSelect>();
   mMenuNameSelect = std::make_shared<MenuScreenNameSelect>();
   mMenuOptions = std::make_shared<MenuScreenOptions>();
   mMenuAudio = std::make_shared<MenuScreenAudio>();
   mMenuVideo = std::make_shared<MenuScreenVideo>();
   mMenuControls = std::make_shared<MenuScreenControls>();
   mMenuGame = std::make_shared<MenuScreenGame>();
   mMenuAchievements = std::make_shared<MenuScreenAchievements>();
   mMenuCredits = std::make_shared<MenuScreenCredits>();
   mMenuPause = std::make_shared<MenuScreenPause>();

   mMenus.push_back(mMenuMain);
   mMenus.push_back(mMenuFileSelect);
   mMenus.push_back(mMenuNameSelect);
   mMenus.push_back(mMenuOptions);
   mMenus.push_back(mMenuAudio);
   mMenus.push_back(mMenuVideo);
   mMenus.push_back(mMenuControls);
   mMenus.push_back(mMenuGame);
   mMenus.push_back(mMenuAchievements);
   mMenus.push_back(mMenuCredits);
   mMenus.push_back(mMenuPause);

   for (auto& screen : mMenus)
   {
      screen->load();
   }
}


std::shared_ptr<Menu>& Menu::getInstance()
{
   if (!sInstance)
   {
      sInstance = std::make_shared<Menu>();
      sInstance->initialize();
   }

   return sInstance;
}


void Menu::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto gameConfig = GameConfiguration::getInstance();

   auto w = gameConfig._view_width;
   auto h = gameConfig._view_height;

   // set up an ortho view with screen dimensions
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));

   window.setView(view);

   if (mCurrentMenu)
   {
      mCurrentMenu->draw(window, states);
   }
}


void Menu::update(const sf::Time& dt)
{
   mCurrentMenu->update(dt);
}


void Menu::show(Menu::MenuType menu)
{
   switch (menu)
   {
      case MenuType::None:
         mCurrentMenu = nullptr;
         break;
      case MenuType::Main:
         mCurrentMenu = mMenuMain;
         break;
      case MenuType::Options:
         mCurrentMenu = mMenuOptions;
         break;
      case MenuType::FileSelect:
         mCurrentMenu = mMenuFileSelect;
         break;
      case MenuType::NameSelect:
         mCurrentMenu = mMenuNameSelect;
         break;
      case MenuType::Controls:
         mCurrentMenu = mMenuControls;
         break;
      case MenuType::Video:
         mCurrentMenu = mMenuVideo;
         break;
      case MenuType::Audio:
         mCurrentMenu = mMenuAudio;
         break;
      case MenuType::Game:
         mCurrentMenu = mMenuGame;
         break;
      case MenuType::Achievements:
         mCurrentMenu = mMenuAchievements;
         break;
      case MenuType::Credits:
         mCurrentMenu = mMenuCredits;
         break;
      case MenuType::Pause:
         mCurrentMenu = mMenuPause;
         break;
   }

   mPreviousType = mCurrentType;
   mCurrentType = menu;

   if (mCurrentMenu)
   {
      mCurrentMenu->showEvent();
      DisplayMode::getInstance().enqueueSet(Display::MainMenu);
   }

   mHistory.push_back(menu);
   while (mHistory.size() > 10)
   {
      mHistory.pop_front();
   }
}


void Menu::hide()
{
   if (mCurrentMenu)
   {
      mCurrentMenu->hideEvent();
   }

   DisplayMode::getInstance().enqueueUnset(Display::MainMenu);

   mCurrentMenu = nullptr;
   mCurrentType = MenuType::None;
}


void Menu::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (mCurrentMenu == nullptr)
   {
      return;
   }

   mCurrentMenu->keyboardKeyPressed(key);
}


void Menu::keyboardKeyReleased(sf::Keyboard::Key key)
{
   if (mCurrentMenu == nullptr)
   {
      return;
   }

   mCurrentMenu->keyboardKeyReleased(key);
}


void Menu::controllerButtonX()
{
   if (mCurrentMenu == nullptr)
   {
      return;
   }

   mCurrentMenu->controllerButtonX();
}


void Menu::controllerButtonY()
{
   if (mCurrentMenu == nullptr)
   {
      return;
   }

   mCurrentMenu->controllerButtonY();
}


bool Menu::isVisible() const
{
   return (mCurrentType != MenuType::None);
}


Menu::MenuType Menu::getCurrentType() const
{
   return mCurrentType;
}


Menu::MenuType Menu::getPreviousType() const
{
   return  mPreviousType;
}


const std::deque<Menu::MenuType>& Menu::getHistory() const
{
   return mHistory;
}


const std::shared_ptr<MenuScreen>& Menu::getMenuScreen(Menu::MenuType type) const
{
    switch (type)
    {
       case MenuType::Main:
          return mMenuMain;
       case MenuType::Options:
          return mMenuOptions;
       case MenuType::FileSelect:
          return mMenuFileSelect;
       case MenuType::NameSelect:
          return mMenuNameSelect;
       case MenuType::Controls:
          return mMenuControls;
       case MenuType::Video:
          return mMenuVideo;
       case MenuType::Audio:
          return mMenuAudio;
       case MenuType::Game:
          return mMenuGame;
       case MenuType::Achievements:
          return mMenuAchievements;
       case MenuType::Credits:
          return mMenuCredits;
       case MenuType::Pause:
          return mMenuPause;
        case MenuType::None:
          break;
    }

    return mMenuInvalid;
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

