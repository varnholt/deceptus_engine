#pragma once

#include "framework/joystick/gamecontrollerinfo.h"

#include "ingamemenuinventory.h"
#include "ingamemenumap.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <memory>
#include <vector>

struct InventoryItem;

class InGameMenu
{
public:
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void left();
   void right();
   void show();
   void hide();
   void confirm();
   void cancel();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:
   void updateControllerActions();
   bool isControllerActionSkipped() const;

   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   InGameMenuInventory _inventory;
   IngameMenuMap _map;
};
