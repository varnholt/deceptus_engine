#include "menu.h"

#include "game/gameconfiguration.h"

#include "menuscreenmain.h"
#include "menuscreenoptions.h"
#include "menuscreenaudio.h"
#include "menuscreencontrols.h"
#include "menuscreenvideo.h"
#include "menuscreengame.h"
#include "menuscreenachievements.h"
#include "menuscreencredits.h"

#include "game/gamecontrollerintegration.h"
#include "joystick/gamecontroller.h"

#include <iostream>


Menu Menu::sInstance;


Menu::Menu()
{
   mMenuMain = std::make_shared<MenuScreenMain>();
   mMenuOptions = std::make_shared<MenuScreenOptions>();
   mMenuAudio = std::make_shared<MenuScreenAudio>();
   mMenuVideo = std::make_shared<MenuScreenVideo>();
   mMenuControls = std::make_shared<MenuScreenControls>();
   mMenuGame = std::make_shared<MenuScreenGame>();
   mMenuAchievements = std::make_shared<MenuScreenAchievements>();
   mMenuCredits = std::make_shared<MenuScreenCredits>();

   mMenus.push_back(mMenuMain);
   mMenus.push_back(mMenuOptions);
   mMenus.push_back(mMenuAudio);
   mMenus.push_back(mMenuVideo);
   mMenus.push_back(mMenuControls);
   mMenus.push_back(mMenuGame);
   mMenus.push_back(mMenuAchievements);
   mMenus.push_back(mMenuCredits);

   for (auto& screen : mMenus)
   {
      screen->load();
   }
}


Menu& Menu::getInstance()
{
   return sInstance;
}


void Menu::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto gameConfig = GameConfiguration::getInstance();

   auto w = gameConfig.mViewWidth;
   auto h = gameConfig.mViewHeight;

   // int32_t textureWidth = (gameConfig.mVideoModeWidth / gameConfig.mViewWidth) * gameConfig.mViewWidth;
   // int32_t textureHeight = (gameConfig.mVideoModeHeight / gameConfig.mViewHeight) * gameConfig.mViewHeight;
   // auto ratioX = textureWidth / static_cast<float>(gameConfig.mVideoModeWidth);
   // auto ratioY = textureHeight / static_cast<float>(gameConfig.mVideoModeHeight);

   // set up an ortho view with screen dimensions
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   // view.setViewport(sf::FloatRect(0.f, 0.f, ratioX, ratioY));

   window.setView(view);

   if (mCurrentMenu)
   {
      mCurrentMenu->draw(window, states);
   }
}


void Menu::update(float dt)
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
   }

   mCurrentType = menu;
}


void Menu::hide()
{
   mCurrentMenu = nullptr;
   mCurrentType = MenuType::None;
}


void Menu::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (mCurrentMenu != nullptr)
   {
      mCurrentMenu->keyboardKeyPressed(key);
   }
}


void Menu::keyboardKeyReleased(sf::Keyboard::Key key)
{
   if (mCurrentMenu != nullptr)
   {
      mCurrentMenu->keyboardKeyReleased(key);
   }
}


Menu::MenuType Menu::getCurrentType() const
{
    return mCurrentType;
}


const std::shared_ptr<MenuScreen>& Menu::getMenuScreen(Menu::MenuType type) const
{
    switch (type)
    {
       case MenuType::Main:
          return mMenuMain;
       case MenuType::Options:
          return mMenuOptions;
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
        case MenuType::None:
          break;
    }
    return nullptr;
}

