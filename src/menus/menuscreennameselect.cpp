#include "menuscreennameselect.h"

#include "menu.h"


MenuScreenNameSelect::MenuScreenNameSelect()
{
   setFilename("data/menus/nameselect.psd");
}


void MenuScreenNameSelect::up()
{

}


void MenuScreenNameSelect::down()
{

}


void MenuScreenNameSelect::select()
{

}


void MenuScreenNameSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
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


void MenuScreenNameSelect::updateLayers()
{
}


