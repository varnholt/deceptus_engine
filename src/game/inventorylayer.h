#ifndef INVENTORYLAYER_H
#define INVENTORYLAYER_H

#include "constants.h"
#include "joystick/gamecontrollerinfo.h"

#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>

struct InventoryItem;

class InventoryLayer
{

public:

   InventoryLayer();

   void addDemoInventory();

   void draw(sf::RenderTarget& window);
   void update(float dt);

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
   std::vector<std::shared_ptr<InventoryItem>>* getInventory();
   void initializeController();
   void updateControllerActions();
   bool isControllerActionSkipped() const;

   sf::Sprite mCursorSprite;
   sf::Vector2f mCursorPosition;
   sf::Texture mInventuryTexture;

   int32_t mSelectedItem = 0;
   bool mActive = false;
   GameControllerInfo mJoystickInfo;
   float mJoystickUpdateTime = 0.0f;
};

#endif // INVENTORYLAYER_H






