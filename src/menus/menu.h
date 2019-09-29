#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "menuscreen.h"

class Menu
{
public:

   enum class MenuType {
      None,
      FileSelect,
      NameSelect,
      Main,
      Options,
      Controls,
      Video,
      Audio,
      Game,
      Achievements,
      Credits,
      Pause
   };


   Menu();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void show(MenuType menu);
   void hide();

   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);
   void controllerButtonX();
   void controllerButtonY();

   bool isVisible() const;

   MenuType getCurrentType() const;
   MenuType getPreviousType() const;

   const std::deque<MenuType>& getHistory() const;

   const std::shared_ptr<MenuScreen>& getMenuScreen(MenuType) const;

   void initialize();

   static std::shared_ptr<Menu>& getInstance();


private:

   MenuType mCurrentType = MenuType::None;
   MenuType mPreviousType = MenuType::None;

   std::deque<MenuType> mHistory;

   std::shared_ptr<MenuScreen> mCurrentMenu;

   std::shared_ptr<MenuScreen> mMenuMain;
   std::shared_ptr<MenuScreen> mMenuFileSelect;
   std::shared_ptr<MenuScreen> mMenuNameSelect;
   std::shared_ptr<MenuScreen> mMenuOptions;
   std::shared_ptr<MenuScreen> mMenuAudio;
   std::shared_ptr<MenuScreen> mMenuControls;
   std::shared_ptr<MenuScreen> mMenuVideo;
   std::shared_ptr<MenuScreen> mMenuGame;
   std::shared_ptr<MenuScreen> mMenuAchievements;
   std::shared_ptr<MenuScreen> mMenuCredits;
   std::shared_ptr<MenuScreen> mMenuPause;
   std::shared_ptr<MenuScreen> mMenuInvalid;

   std::vector<std::shared_ptr<MenuScreen>> mMenus;

   static std::shared_ptr<Menu> sInstance;
};

