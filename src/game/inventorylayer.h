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

   sf::Sprite mCursorSprite;
   sf::Vector2f mCursorPosition;
   sf::Texture mInventuryTexture;

   std::map<ItemType, ItemSprite> mSprites;
   int32_t mSelectedItem = 0;
   bool mActive = false;
   GameControllerInfo mJoystickInfo;
   float mJoystickUpdateTime = 0.0f;
};

