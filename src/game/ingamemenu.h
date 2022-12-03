#pragma once

#include "framework/joystick/gamecontrollerinfo.h"

#include "ingamemenuarchives.h"
#include "ingamemenuinventory.h"
#include "ingamemenumap.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

struct InventoryItem;

class InGameMenu
{
public:
   enum class SubMenu
   {
      Map = 0,
      Inventory = 1,
      Archives = 2,
   };

   InGameMenu();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void processEvent(const sf::Event& event);

   void open();
   void close();

   void left();
   void right();
   void show();
   void hide();

   void confirm();
   void cancel();

   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:

   void initializeController();
   void updateController();

   void goToRightSubMenu();
   void goToLeftSubMenu();
   void debug();

   void updateControllerActions();
   bool isControllerActionSkipped() const;

   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   std::shared_ptr<InGameMenuArchives> _menu_archives;
   std::shared_ptr<InGameMenuInventory> _menu_inventory;
   std::shared_ptr<IngameMenuMap> _menu_map;

   SubMenu _selected_submenu{SubMenu::Inventory};
   std::optional<SubMenu> _previous_submenu;

   std::array<SubMenu, 3> _submenu_selection;
   std::array<std::shared_ptr<InGameMenuPage>, 3> _submenu_type_map;
   std::array<std::string, 3> _submenu_type_names;
};
