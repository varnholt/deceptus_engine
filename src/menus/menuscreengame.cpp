#include "menuscreengame.h"

#include "menu.h"


MenuScreenGame::MenuScreenGame()
{
   setFilename("data/menus/game.psd");
}


void MenuScreenGame::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenGame::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   _selection = static_cast<Selection>(next);
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
   auto autoPause = _selection == Selection::AutomaticPause;
   auto textSpeed = _selection == Selection::TextSpeed;

   auto autoPauseSelection = 0;
   auto textSpeedSelection = 0;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["autoPause_text_0"]->_visible = !autoPause;
   _layers["autoPause_text_1"]->_visible = autoPause;
   _layers["autoPause_highlight"]->_visible = autoPause;
   _layers["autoPause_help"]->_visible = autoPause;
   _layers["autoPause_arrows"]->_visible = autoPause;
   _layers["autoPause_value_no"]->_visible = autoPauseSelection == 0;
   _layers["autoPause_value_yes"]->_visible = autoPauseSelection == 1;

   _layers["textSpeed_text_0"]->_visible = !textSpeed;
   _layers["textSpeed_text_1"]->_visible = textSpeed;
   _layers["textSpeed_highlight"]->_visible = textSpeed;
   _layers["textSpeed_help"]->_visible = textSpeed;
   _layers["textSpeed_arrows"]->_visible = textSpeed;
   _layers["textSpeed_1"]->_visible = textSpeedSelection == 0;
   _layers["textSpeed_2"]->_visible = textSpeedSelection == 1;
   _layers["textSpeed_3"]->_visible = textSpeedSelection == 2;
   _layers["textSpeed_4"]->_visible = textSpeedSelection == 3;
   _layers["textSpeed_5"]->_visible = textSpeedSelection == 4;
}


/*
data/menus/game.psd
    bg_temp

    game-window-bg
    game_window-main

    header
*/
