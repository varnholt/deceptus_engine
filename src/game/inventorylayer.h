#pragma once

#include "constants.h"
#include "framework/image/layer.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "inventory.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

struct InventoryItem;

class InventoryLayer
{
public:
   struct ItemSprite
   {
      sf::Sprite mSprite;
   };

   InventoryLayer();

   void addDemoInventory();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void left();
   void right();
   void show();
   void hide();
   void setActive(bool active);
   void confirm();
   void cancel();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:
   void addItem(int32_t x, int32_t y, ItemType type);
   Inventory& getInventory();
   void updateControllerActions();
   bool isControllerActionSkipped() const;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   sf::Font _font;
   sf::Text _text;

   sf::Sprite _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;

   std::map<ItemType, ItemSprite> _sprites;
   int32_t _selected_item = 0;
   bool _active = false;
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;
};
