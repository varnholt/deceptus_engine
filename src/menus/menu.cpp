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
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   // set up an ortho view with screen dimensions
   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
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

