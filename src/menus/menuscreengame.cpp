#include "menuscreengame.h"

#include "menu.h"


MenuScreenGame::MenuScreenGame()
{
   setFilename("data/menus/game.psd");
}


void MenuScreenGame::up()
{
   auto next = static_cast<int32_t>(mSelection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenGame::down()
{
   auto next = static_cast<int32_t>(mSelection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenGame::select()
{

}


void MenuScreenGame::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenGame::keyboardKeyPressed(sf::Keyboard::Key key)
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


void MenuScreenGame::loadingFinished()
{
   updateLayers();
}


void MenuScreenGame::updateLayers()
{
   auto autoPause = mSelection == Selection::AutomaticPause;
   auto textSpeed = mSelection == Selection::TextSpeed;

   auto autoPauseSelection = 0;
   auto textSpeedSelection = 0;

   mLayers["defaults_xbox_0"]->_visible = isControllerUsed();
   mLayers["defaults_xbox_1"]->_visible = false;
   mLayers["back_xbox_0"]->_visible = isControllerUsed();
   mLayers["back_xbox_1"]->_visible = false;

   mLayers["defaults_pc_0"]->_visible = !isControllerUsed();
   mLayers["defaults_pc_1"]->_visible = false;
   mLayers["back_pc_0"]->_visible = !isControllerUsed();
   mLayers["back_pc_1"]->_visible = false;

   mLayers["autoPause_text_0"]->_visible = !autoPause;
   mLayers["autoPause_text_1"]->_visible = autoPause;
   mLayers["autoPause_highlight"]->_visible = autoPause;
   mLayers["autoPause_help"]->_visible = autoPause;
   mLayers["autoPause_arrows"]->_visible = autoPause;
   mLayers["autoPause_value_no"]->_visible = autoPauseSelection == 0;
   mLayers["autoPause_value_yes"]->_visible = autoPauseSelection == 1;

   mLayers["textSpeed_text_0"]->_visible = !textSpeed;
   mLayers["textSpeed_text_1"]->_visible = textSpeed;
   mLayers["textSpeed_highlight"]->_visible = textSpeed;
   mLayers["textSpeed_help"]->_visible = textSpeed;
   mLayers["textSpeed_arrows"]->_visible = textSpeed;
   mLayers["textSpeed_1"]->_visible = textSpeedSelection == 0;
   mLayers["textSpeed_2"]->_visible = textSpeedSelection == 1;
   mLayers["textSpeed_3"]->_visible = textSpeedSelection == 2;
   mLayers["textSpeed_4"]->_visible = textSpeedSelection == 3;
   mLayers["textSpeed_5"]->_visible = textSpeedSelection == 4;
}


/*
data/menus/game.psd
    bg_temp

    game-window-bg
    game_window-main

    header
*/
