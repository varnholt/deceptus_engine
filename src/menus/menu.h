#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "menuscreen.h"

/// \brief owns all menu screens and routes menu input to the currently shown screen.
class Menu
{
public:
   /// \brief identifies the available menu screens.
   enum class MenuType
   {
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

   /// \brief creates all menu screens, loads their assets, and initializes shared menu sounds.
   Menu();

   /// \brief draws the active menu screen using a view sized to the configured game resolution.
   /// \param window render target that receives menu layers.
   /// \param states render states forwarded to the active screen.
   void draw(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);

   /// \brief updates the currently visible menu screen.
   /// \param dt frame delta time passed to the active screen.
   void update(const sf::Time& dt);

   /// \brief switches to a specific menu screen, triggers its show hook, and records it in history.
   /// \param menu menu type to activate.
   void show(MenuType menu);

   /// \brief hides the current menu screen and removes the main-menu display flag.
   void hide();

   /// \brief forwards a keyboard press to the active menu screen.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key);

   /// \brief forwards a keyboard release to the active menu screen.
   /// \param key key that was released.
   void keyboardKeyReleased(sf::Keyboard::Key key);

   /// \brief forwards the controller X action to the active menu screen.
   void controllerButtonX();

   /// \brief forwards the controller Y action to the active menu screen.
   void controllerButtonY();

   /// \brief checks whether a menu screen is currently visible.
   /// \return true when the current menu type is not None.
   bool isVisible() const;

   /// \brief returns the currently selected menu type.
   /// \return type of the active menu screen.
   MenuType getCurrentType() const;

   /// \brief returns the menu type that was active before the current one.
   /// \return previously shown menu type.
   MenuType getPreviousType() const;

   /// \brief returns recent menu transitions.
   /// \return deque containing up to the last ten shown menu types.
   const std::deque<MenuType>& getHistory() const;

   /// \brief retrieves the screen instance mapped to a menu type.
   /// \param type menu type to resolve.
   /// \return shared pointer to the matching screen, or an invalid placeholder for None.
   const std::shared_ptr<MenuScreen>& getMenuScreen(MenuType type) const;

   /// \brief binds gamepad axis and button callbacks to menu navigation actions.
   void initialize();

   /// \brief returns the singleton menu instance, creating and initializing it on first use.
   /// \return shared pointer reference to the global menu manager.
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
   std::shared_ptr<MenuScreen> _menu_video;
   std::shared_ptr<MenuScreen> _menu_controls;
   std::shared_ptr<MenuScreen> _menu_game;
   std::shared_ptr<MenuScreen> _menu_achievements;
   std::shared_ptr<MenuScreen> _menu_credits;
   std::shared_ptr<MenuScreen> _menu_pause;
   std::shared_ptr<MenuScreen> _menu_invalid;

   std::vector<std::shared_ptr<MenuScreen>> _menus;

   static std::shared_ptr<Menu> __instance;
};
