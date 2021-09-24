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

   MenuType _current_type = MenuType::None;
   MenuType _previous_type = MenuType::None;

   std::deque<MenuType> _history;

   std::shared_ptr<MenuScreen> _current_menu;

   std::shared_ptr<MenuScreen> _menu_main;
   std::shared_ptr<MenuScreen> _menu_file_select;
   std::shared_ptr<MenuScreen> _menu_name_select;
   std::shared_ptr<MenuScreen> _menu_options;
   std::shared_ptr<MenuScreen> _menu_audio;
   std::shared_ptr<MenuScreen> _menu_controls;
   std::shared_ptr<MenuScreen> _menu_video;
   std::shared_ptr<MenuScreen> _menu_game;
   std::shared_ptr<MenuScreen> _menu_achievements;
   std::shared_ptr<MenuScreen> _menu_credits;
   std::shared_ptr<MenuScreen> _menu_pause;
   std::shared_ptr<MenuScreen> _menu_invalid;

   std::vector<std::shared_ptr<MenuScreen>> _menus;

   static std::shared_ptr<Menu> __instance;
};

