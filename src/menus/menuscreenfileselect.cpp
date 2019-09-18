#include "menuscreenfileselect.h"

#include "menu.h"


MenuScreenFileSelect::MenuScreenFileSelect()
{
   setFilename("data/menus/fileselect.psd");
}


void MenuScreenFileSelect::up()
{

}


void MenuScreenFileSelect::down()
{

}


void MenuScreenFileSelect::select()
{

}


void MenuScreenFileSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
}


void MenuScreenFileSelect::keyboardKeyPressed(sf::Keyboard::Key key)
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


void MenuScreenFileSelect::loadingFinished()
{
   updateLayers();
}


void MenuScreenFileSelect::updateLayers()
{
}


