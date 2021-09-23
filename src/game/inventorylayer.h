#pragma once

#include "constants.h"
#include "inventory.h"
#include "framework/joystick/gamecontrollerinfo.h"

#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>

struct InventoryItem;

class InventoryLayer
{

public:

   struct ItemSprite {
      sf::Sprite mSprite;
   };

   InventoryLayer();

   void addDemoInventory();

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& dt);

   void left();
   void right();
   void show();
   void hide();
   void setActive(bool active);
   void confirm();
   void cancel();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo &joystickInfo);


private:

   void addItem(int32_t x, int32_t y, ItemType type);
   Inventory& getInventory();
   void updateControllerActions();
   bool isControllerActionSkipped() const;

   sf::Sprite _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;

   std::map<ItemType, ItemSprite> _sprites;
   int32_t _selected_item = 0;
   bool _active = false;
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;
};

