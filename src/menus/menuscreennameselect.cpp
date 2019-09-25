#include "menuscreennameselect.h"

#include "menu.h"
#include "game/gamestate.h"

#include <iostream>

namespace {
   const int32_t charWidth = 19;
   const int32_t charHeight = 24;
}


MenuScreenNameSelect::MenuScreenNameSelect()
{
   mFont.loadFromFile("data/fonts/deceptum.ttf");

   mText.setScale(0.25f, 0.25f);
   mText.setFont(mFont);
   mText.setCharacterSize(48);
   mText.setFillColor(sf::Color{232, 219, 243});

   setFilename("data/menus/nameselect.psd");

   mChars = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '.', '-',
   };
}


void MenuScreenNameSelect::up()
{
   mCharOffset.y--;
   mCharOffset.y = std::max(mCharOffset.y, 0);
   updateLayers();
}


void MenuScreenNameSelect::down()
{
   mCharOffset.y++;
   mCharOffset.y = std::min(mCharOffset.y, 4);
   updateLayers();
}

void MenuScreenNameSelect::left()
{
   mCharOffset.x--;
   mCharOffset.x = std::max(mCharOffset.x, 0);
   updateLayers();
}

void MenuScreenNameSelect::right()
{
   mCharOffset.x++;
   mCharOffset.x = std::min(mCharOffset.x, 12);
   updateLayers();
}


void MenuScreenNameSelect::select()
{
   Menu::getInstance()->hide();
   GameState::getInstance().enqueueResume();
}


void MenuScreenNameSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::FileSelect);
}


void MenuScreenNameSelect::keyboardKeyPressed(sf::Keyboard::Key key)
{
   mShift += static_cast<int32_t>(key == sf::Keyboard::LShift);
   mShift += static_cast<int32_t>(key == sf::Keyboard::RShift);

   if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z)
   {
      if (mName.size() > 10)
      {
         return;
      }

      mName += mChars[static_cast<uint32_t>(key + (mShift ? 0 : 26))];
      updateLayers();
   }

   if (key == sf::Keyboard::Delete || key == sf::Keyboard::Backspace)
   {
      if (mName.empty())
      {
         return;
      }

      mName.pop_back();
      updateLayers();
   }

   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
   }

   if (key == sf::Keyboard::Left)
   {
      left();
   }

   else if (key == sf::Keyboard::Right)
   {
      right();
   }

   else if (key == sf::Keyboard::Return)
   {
      select();
   }

   else if (key == sf::Keyboard::Escape)
   {
      back();
   }
}


void MenuScreenNameSelect::keyboardKeyReleased(sf::Keyboard::Key key)
{
   mShift -= static_cast<int32_t>(key == sf::Keyboard::LShift);
   mShift -= static_cast<int32_t>(key == sf::Keyboard::RShift);
}


void MenuScreenNameSelect::loadingFinished()
{
    auto cursor = mLayers["cursor"];
    mCharOrigin.x = cursor->mSprite->getPosition().x;
    mCharOrigin.y = cursor->mSprite->getPosition().y;

    auto playerName = mLayers["players-name"];
    mNameRect.left = playerName->mSprite->getPosition().x;
    mNameRect.top = playerName->mSprite->getPosition().y;
    mNameRect.width = playerName->mTexture->getSize().x;

    updateLayers();
}


void MenuScreenNameSelect::updateLayers()
{
   // for (auto& layer : mLayers)
   // {
   //    std::cout << layer.first << std::endl;
   // }

   auto cursor = mLayers["cursor"];
   cursor->mSprite->setPosition(
      static_cast<float>(mCharOrigin.x + mCharOffset.x * charWidth),
      static_cast<float>(mCharOrigin.y + mCharOffset.y * charHeight)
   );

   mLayers["header-bg"]->mVisible = true;
   mLayers["players-name"]->mVisible = false;
   mLayers["temp_bg"]->mVisible = true;
   mLayers["title"]->mVisible = true;
   mLayers["window"]->mVisible = true;

   mLayers["name-error-msg"]->mVisible = false;
   mLayers["Please enter your name"]->mVisible = true;

   mLayers["delete_xbox_0"]->mVisible = isControllerUsed();
   mLayers["delete_xbox_1"]->mVisible = false;
   mLayers["delete_pc_0"]->mVisible = !isControllerUsed();
   mLayers["delete_pc_1"]->mVisible = false;

   mLayers["confirm_xbox_0"]->mVisible = isControllerUsed();
   mLayers["confirm_xbox_1"]->mVisible = false;
   mLayers["confirm_pc_0"]->mVisible = !isControllerUsed();
   mLayers["confirm_pc_1"]->mVisible = false;

   mLayers["cancel_xbox_0"]->mVisible = isControllerUsed();
   mLayers["cancel_xbox_1"]->mVisible = false;
   mLayers["cancel_pc_0"]->mVisible = !isControllerUsed();
   mLayers["cancel_pc_1"]->mVisible = false;
}


void MenuScreenNameSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   // draw text
   const auto textRect = mText.getGlobalBounds();
   const auto x = mNameRect.left + (mNameRect.width - textRect.width) * 0.5f;
   mText.setString(mName);
   mText.setPosition(x, mNameRect.top);
   window.draw(mText, states);
}


