#include "menuscreenachievements.h"

#include "menu.h"


MenuScreenAchievements::MenuScreenAchievements()
{
   setFilename("data/menus/achievements.psd");
}


void MenuScreenAchievements::up()
{

}


void MenuScreenAchievements::down()
{

}


void MenuScreenAchievements::select()
{

}


void MenuScreenAchievements::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenAchievements::keyboardKeyPressed(sf::Keyboard::Key key)
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


void MenuScreenAchievements::loadingFinished()
{
   updateLayers();
}


void MenuScreenAchievements::updateLayers()
{
   mLayers["back_xbox_0"]->mVisible = isControllerUsed();
   mLayers["back_xbox_1"]->mVisible = false;

   mLayers["back_pc_0"]->mVisible = !isControllerUsed();
   mLayers["back_pc_1"]->mVisible = false;

}


/*
data/menus/achievements.psd
    bg_tmp
    back_xbox_0
    back_xbox_1
    back_pc_0
    back_pc_1
    title
*/

