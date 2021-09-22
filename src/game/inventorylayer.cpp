#include "inventorylayer.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"
#include "gamecontrollerintegration.h"

#include "player/player.h"
#include "player/playerinfo.h"
#include "extramanager.h"
#include "inventoryitem.h"
#include "savestate.h"
#include "texturepool.h"

namespace {
   static const auto iconWidth  = 40;
   static const auto iconHeight = 24;
   static const auto quadWidth  = 38;
   static const auto quadHeight = 38;
   static const auto dist = 10.2f;
   static const auto iconQuadDist = (iconWidth - quadWidth);
   static const auto yOffset = 300.0f;
   static const auto itemCount = 13;
}


//---------------------------------------------------------------------------------------------------------------------
GameControllerInfo InventoryLayer::getJoystickInfo() const
{
   return mJoystickInfo;
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::setJoystickInfo(const GameControllerInfo &joystickInfo)
{
   mJoystickInfo = joystickInfo;
}


//---------------------------------------------------------------------------------------------------------------------
InventoryLayer::InventoryLayer()
{
   mInventuryTexture = TexturePool::getInstance().get("data/game/inventory.png");
   mCursorSprite.setTexture(*mInventuryTexture);
   mCursorSprite.setTextureRect({0, 512 - 48, 48, 48});
   addDemoInventory();
}


//---------------------------------------------------------------------------------------------------------------------
Inventory& InventoryLayer::getInventory()
{
   return SaveState::getPlayerInfo().mInventory;
}



//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::addItem(int32_t x, int32_t y, ItemType type)
{
   sf::Sprite sprite;
   sprite.setTexture(*mInventuryTexture);
   sprite.setTextureRect({x * iconWidth, y * iconHeight, iconWidth, iconHeight});
   mSprites[type].mSprite = sprite;

   getInventory().add(type);
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::addDemoInventory()
{
   addItem(0,  0, ItemType::KeyRed   );
   addItem(1,  0, ItemType::KeyOrange);
   addItem(2,  0, ItemType::KeyBlue  );
   addItem(3,  0, ItemType::KeyGreen );
   addItem(4,  0, ItemType::KeyYellow);
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::draw(sf::RenderTarget &window)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   const sf::Color color = {50, 70, 100, 150};

   auto x = dist;
   auto y = yOffset + 5.0f;

   for (int i = 0; i < itemCount; i++)
   {
      sf::Vertex quad[] =
      {
         sf::Vertex(sf::Vector2f(static_cast<float>(x),                                 static_cast<float>(y)                                 ), color),
         sf::Vertex(sf::Vector2f(static_cast<float>(x),                                 static_cast<float>(y) + static_cast<float>(quadHeight)), color),
         sf::Vertex(sf::Vector2f(static_cast<float>(x) + static_cast<float>(quadWidth), static_cast<float>(y) + static_cast<float>(quadHeight)), color),
         sf::Vertex(sf::Vector2f(static_cast<float>(x) + static_cast<float>(quadWidth), static_cast<float>(y)                                 ), color)
      };

      window.draw(quad, 4, sf::Quads);
      x += quadWidth + dist;
   }

   y = yOffset  + 15.0f;
   x = dist;

   for (auto item : SaveState::getPlayerInfo().mInventory.getItems())
   {
      auto visualization = mSprites[item.mType];

      visualization.mSprite.setPosition(static_cast<float>(x), static_cast<float>(y));
      window.draw(visualization.mSprite);
      x += iconWidth + dist - iconQuadDist;
   }

   mCursorPosition.y = yOffset;
   mCursorSprite.setPosition(mCursorPosition);
   window.draw(mCursorSprite);
}


//---------------------------------------------------------------------------------------------------------------------
bool InventoryLayer::isControllerActionSkipped() const
{
   auto skipped = false;
   auto now = GlobalClock::getInstance()->getElapsedTimeInS();

   if (now - mJoystickUpdateTime < 0.3f)
   {
      skipped = true;
   }

   return skipped;
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::updateControllerActions()
{
  auto gji =GameControllerIntegration::getInstance(0);

  if (gji == nullptr)
  {
    return;
  }

  auto axisValues = mJoystickInfo.getAxisValues();
  auto axisLeftX = gji->getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
  auto xl = axisValues[axisLeftX] / 32767.0f;
  auto hatValue = mJoystickInfo.getHatValues().at(0);
  auto dpadLeftPressed = hatValue & SDL_HAT_LEFT;
  auto dpadRightPressed = hatValue & SDL_HAT_RIGHT;
  if (dpadLeftPressed)
  {
     xl = -1.0f;
  }
  else if (dpadRightPressed)
  {
     xl = 1.0f;
  }

  if (fabs(xl)> 0.3f)
  {
     if (xl < 0.0f)
     {
       if (!isControllerActionSkipped())
       {
         mJoystickUpdateTime = GlobalClock::getInstance()->getElapsedTimeInS();
         left();
       }
     }
     else
     {
       if (!isControllerActionSkipped())
       {
         mJoystickUpdateTime = GlobalClock::getInstance()->getElapsedTimeInS();
         right();
       }
     }
  }
  else
  {
    mJoystickUpdateTime = 0.0f;
  }
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::update(const sf::Time& /*dt*/)
{
  mCursorPosition.x = dist * 0.5f + mSelectedItem * (quadWidth + dist) - 0.5f;
  updateControllerActions();
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::left()
{
  if (!mActive)
  {
    return;
  }

  if (mSelectedItem > 0)
  {
    mSelectedItem--;
  }
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::right()
{
   if (!mActive)
   {
      return;
   }

   if (mSelectedItem < static_cast<int32_t>(getInventory().getItems().size()) - 1)
   {
      mSelectedItem++;
   }
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::show()
{
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::hide()
{
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::setActive(bool active)
{
  mActive = active;
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::confirm()
{
   if (!mActive)
   {
      return;
   }

   hide();
}


//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::cancel()
{
   if (!mActive)
   {
      return;
   }

   hide();
}


