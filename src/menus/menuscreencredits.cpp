#include "menuscreencredits.h"

#include "menu.h"


MenuScreenCredits::MenuScreenCredits()
{
   setFilename("data/menus/credits.psd");
}


void MenuScreenCredits::loadingFinished()
{
   updateLayers();
}


void MenuScreenCredits::updateLayers()
{
   mLayers["back_xbox_0"]->mVisible = isControllerUsed();
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["back_pc_0"]->mVisible = !isControllerUsed();
   mLayers["back_pc_1"]->mVisible = false;
}


void MenuScreenCredits::up()
{

}


void MenuScreenCredits::down()
{

}


void MenuScreenCredits::select()
{

}


void MenuScreenCredits::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenCredits::keyboardKeyPressed(sf::Keyboard::Key key)
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

/*
data/menus/credits.psd
    bg_tmp

*/
