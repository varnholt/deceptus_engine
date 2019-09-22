#include "menuscreennameselect.h"

#include "menu.h"
#include "game/gamestate.h"

#include <iostream>


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

}


void MenuScreenNameSelect::down()
{

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
   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
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
   updateLayers();
}


/*

cursor

*/


void MenuScreenNameSelect::updateLayers()
{
   // for (auto& layer : mLayers)
   // {
   //    std::cout << layer.first << std::endl;
   // }

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


