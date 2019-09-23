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


/*
        A = 0,        ///< The A key
        B,            ///< The B key
        C,            ///< The C key
        D,            ///< The D key
        E,            ///< The E key
        F,            ///< The F key
        G,            ///< The G key
        H,            ///< The H key
        I,            ///< The I key
        J,            ///< The J key
        K,            ///< The K key
        L,            ///< The L key
        M,            ///< The M key
        N,            ///< The N key
        O,            ///< The O key
        P,            ///< The P key
        Q,            ///< The Q key
        R,            ///< The R key
        S,            ///< The S key
        T,            ///< The T key
        U,            ///< The U key
        V,            ///< The V key
        W,            ///< The W key
        X,            ///< The X key
        Y,            ///< The Y key
        Z,            ///< The Z key

        LShift,       ///< The left Shift key
        RShift,       ///< The right Shift key
*/

void MenuScreenNameSelect::keyboardKeyPressed(sf::Keyboard::Key key)
{
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


void MenuScreenNameSelect::loadingFinished()
{
    auto cursor = mLayers["cursor"];
    mCharOrigin.x = cursor->mSprite->getPosition().x;
    mCharOrigin.y = cursor->mSprite->getPosition().y;

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
        mCharOrigin.x
      + mCharOffset.x * charWidth,
        mCharOrigin.y
      + mCharOffset.y * charHeight
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


