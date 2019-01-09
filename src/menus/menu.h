#pragma once

#include <memory>
#include <vector>

#include "menuscreen.h"

class Menu
{
public:

   enum class MenuType {
      None,
      Main,
      Options,
      Controls,
      Video,
      Audio,
      Game,
      Achievements,
      Credits
   };

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(float dt);

   void show(MenuType menu);
   void hide();

   void keyboardKeyPressed(sf::Keyboard::Key key);
   void keyboardKeyReleased(sf::Keyboard::Key key);

   MenuType getCurrentType() const;

   static Menu& getInstance();


private:

   Menu();

   MenuType mCurrentType = MenuType::None;

   std::shared_ptr<MenuScreen> mCurrentMenu;

   std::shared_ptr<MenuScreen> mMenuMain;
   std::shared_ptr<MenuScreen> mMenuOptions;
   std::shared_ptr<MenuScreen> mMenuAudio;
   std::shared_ptr<MenuScreen> mMenuControls;
   std::shared_ptr<MenuScreen> mMenuVideo;
   std::shared_ptr<MenuScreen> mMenuGame;
   std::shared_ptr<MenuScreen> mMenuAchievements;
   std::shared_ptr<MenuScreen> mMenuCredits;

   std::vector<std::shared_ptr<MenuScreen>> mMenus;

   static Menu sInstance;
};

