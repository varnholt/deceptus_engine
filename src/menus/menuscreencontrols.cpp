#include "menuscreencontrols.h"

#include "menu.h"
#include "menuaudio.h"

MenuScreenControls::MenuScreenControls()
{
   setFilename("data/menus/controls.psd");
}

void MenuScreenControls::up()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenControls::down()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenControls::select()
{
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenControls::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenControls::keyboardKeyPressed(sf::Keyboard::Key key)
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

void MenuScreenControls::loadingFinished()
{
   updateLayers();
}

void MenuScreenControls::updateLayers()
{
   // mLayers["body"]->mVisible = false;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;

   _layers["setKey_xbox_0"]->_visible = isControllerUsed();
   _layers["setKey_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;

   _layers["setKey_pc_0"]->_visible = !isControllerUsed();
   _layers["setKey_pc_1"]->_visible = false;

   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;
}

/*
data/menus/controls.psd
    bg_temp
    video-window-bg
    video_window-main
    displayMode_arrows
    body
    scrollbar_body
    scrollbar_slider
    vibration_on
    body_header
    header
*/
