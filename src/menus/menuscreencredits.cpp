#include "menuscreencredits.h"

#include "menu.h"
#include "menuaudio.h"

MenuScreenCredits::MenuScreenCredits()
{
   setFilename("data/menus/credits.psd");

   ensureFontLoaded();

   _text_code = std::make_unique<sf::Text>(_font);
   _text_code->setFont(_font);
   _text_code->setString("Code: Matthias Varnholt");
   _text_code->setCharacterSize(12);
   _text_code->setPosition({220.0f, 155.0f});
   _text_code->setFillColor(sf::Color{232, 219, 243});

   _text_artwork = std::make_unique<sf::Text>(_font);
   _text_artwork->setFont(_font);
   _text_artwork->setString("Artwork: dstar");
   _text_artwork->setCharacterSize(12);
   _text_artwork->setPosition({220.0f, 177.0f});
   _text_artwork->setFillColor(sf::Color{232, 219, 243});
}

void MenuScreenCredits::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);
   window.draw(*_text_code);
   window.draw(*_text_artwork);
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
