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
   Menu::getInstance().show(Menu::MenuType::Options);
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

   mLayers["defaults_xbox_0"]->mVisible = false;
   mLayers["defaults_xbox_1"]->mVisible = false;
   mLayers["back_xbox_0"]->mVisible = false;
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["defaults_pc_0"]->mVisible = true;
   mLayers["defaults_pc_1"]->mVisible = false;
   mLayers["back_pc_0"]->mVisible = true;
   mLayers["back_pc_1"]->mVisible = false;

   mLayers["autoPause_text_0"]->mVisible = !autoPause;
   mLayers["autoPause_text_1"]->mVisible = autoPause;
   mLayers["autoPause_highlight"]->mVisible = autoPause;
   mLayers["autoPause_help"]->mVisible = autoPause;
   mLayers["autoPause_arrows"]->mVisible = autoPause;
   mLayers["autoPause_value_no"]->mVisible = autoPauseSelection == 0;
   mLayers["autoPause_value_yes"]->mVisible = autoPauseSelection == 1;

   mLayers["textSpeed_text_0"]->mVisible = !textSpeed;
   mLayers["textSpeed_text_1"]->mVisible = textSpeed;
   mLayers["textSpeed_body_0"]->mVisible = textSpeed;
   mLayers["textSpeed_highlight"]->mVisible = textSpeed;
   mLayers["textSpeed_body_1"]->mVisible = textSpeed;
   mLayers["textSpeed_help"]->mVisible = textSpeed;
   mLayers["textSpeed_arrows"]->mVisible = textSpeed;
   mLayers["textSpeed_1"]->mVisible = textSpeedSelection == 0;
   mLayers["textSpeed_2"]->mVisible = textSpeedSelection == 1;
   mLayers["textSpeed_3"]->mVisible = textSpeedSelection == 2;
   mLayers["textSpeed_4"]->mVisible = textSpeedSelection == 3;
   mLayers["textSpeed_5"]->mVisible = textSpeedSelection == 4;
   mLayers["textSpeed_h"]->mVisible = true;
}


/*
data/menus/game.psd
    bg_temp

    game-window-bg
    game_window-main

    header
*/
