#include "menuscreencredits.h"

#include "menu.h"
#include "menuaudio.h"

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
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;
}

void MenuScreenCredits::up()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenCredits::down()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenCredits::select()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenCredits::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenCredits::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Key::Enter)
   {
      select();
   }

   else if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }
}

/*
data/menus/credits.psd
    bg_tmp

*/
