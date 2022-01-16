#include "menuscreennameselect.h"

#include "menu.h"
#include "game/gamestate.h"
#include "game/savestate.h"

#include <cstdlib>
#include <iostream>
#include <stdlib.h>


namespace {
   static const int32_t charWidth = 19;
   static const int32_t charHeight = 24;
   static const size_t maxLength = 11;
}


MenuScreenNameSelect::MenuScreenNameSelect()
{
   _font.loadFromFile("data/fonts/deceptum.ttf");
   const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);

   _text.setFont(_font);
   _text.setCharacterSize(12);
   _text.setFillColor(sf::Color{232, 219, 243});

   setFilename("data/menus/nameselect.psd");

   _chars = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '.', '-',
   };
}


void MenuScreenNameSelect::up()
{
   _char_offset.y--;
   _char_offset.y = std::max(_char_offset.y, 0);
   updateLayers();
}


void MenuScreenNameSelect::down()
{
   _char_offset.y++;
   _char_offset.y = std::min(_char_offset.y, 4);
   updateLayers();
}

void MenuScreenNameSelect::left()
{
   _char_offset.x--;
   _char_offset.x = std::max(_char_offset.x, 0);
   updateLayers();
}

void MenuScreenNameSelect::right()
{
   _char_offset.x++;
   _char_offset.x = std::min(_char_offset.x, 12);
   updateLayers();
}


void MenuScreenNameSelect::select()
{
   if (_name.empty())
   {
      return;
   }

   Menu::getInstance()->hide();
   GameState::getInstance().enqueueResume();

   SaveState::getCurrent()._player_info._name = _name;

   // request level-reloading since we updated the save state
   SaveState::getCurrent()._load_level_requested = true;
}


void MenuScreenNameSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::FileSelect);
}


void MenuScreenNameSelect::updateText()
{
   // draw text
   _text.setString(_name);
   const auto textRect = _text.getLocalBounds();
   const auto xOffset = (_name_rect.width - textRect.width) * 0.5f;
   const auto x = _name_rect.left + xOffset;
   _text.setPosition(x, _name_rect.top);
}


void MenuScreenNameSelect::chop()
{
   if (_name.empty())
   {
      return;
   }

   _name.pop_back();
   updateText();
   updateLayers();
}


void MenuScreenNameSelect::appendChar(char c)
{
   _name += c;
   updateText();
   updateLayers();
}


void MenuScreenNameSelect::keyboardKeyPressed(sf::Keyboard::Key key)
{
   _shift += static_cast<int32_t>(key == sf::Keyboard::LShift);
   _shift += static_cast<int32_t>(key == sf::Keyboard::RShift);

   if (key >= sf::Keyboard::A && key <= sf::Keyboard::Z)
   {
      if (_name.size() >= maxLength)
      {
         return;
      }

      const auto c = _chars[static_cast<uint32_t>(key + (_shift ? 0 : 26))];
      appendChar(c);
   }

   if (key == sf::Keyboard::Delete || key == sf::Keyboard::Backspace)
   {
      chop();
   }

   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
   }

   if (key == sf::Keyboard::Left)
   {
      left();
   }

   else if (key == sf::Keyboard::Right)
   {
      right();
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


void MenuScreenNameSelect::keyboardKeyReleased(sf::Keyboard::Key key)
{
   _shift -= static_cast<int32_t>(key == sf::Keyboard::LShift);
   _shift -= static_cast<int32_t>(key == sf::Keyboard::RShift);
}


void MenuScreenNameSelect::controllerButtonX()
{
   chop();
}


void MenuScreenNameSelect::controllerButtonY()
{
   char c = _chars[static_cast<size_t>(_char_offset.x + _char_offset.y * 13)];
   appendChar(c);
}


void MenuScreenNameSelect::retrieveUsername()
{
   // probably requires a regular expression to filter out the unicode crap
   auto u1 = std::getenv("USERNAME");
   auto u2 = std::getenv("USER");
   _name = u1 ? u1 : (u2 ? u2 : "");

   if (!_name.empty())
   {
      _name[0] = std::toupper(_name[0]);
      updateText();
   }
}


void MenuScreenNameSelect::loadingFinished()
{
    auto cursor = _layers["cursor"];
    _char_origin.x = cursor->_sprite->getPosition().x;
    _char_origin.y = cursor->_sprite->getPosition().y;

    auto playerName = _layers["players-name"];
    _name_rect.left = playerName->_sprite->getPosition().x;
    _name_rect.top = playerName->_sprite->getPosition().y;
    _name_rect.width = static_cast<float>(playerName->_texture->getSize().x);

    retrieveUsername();

    updateLayers();
}


void MenuScreenNameSelect::updateLayers()
{
   auto cursor = _layers["cursor"];
   cursor->_sprite->setPosition(
      static_cast<float>(_char_origin.x + _char_offset.x * charWidth),
      static_cast<float>(_char_origin.y + _char_offset.y * charHeight)
   );

   _layers["header-bg"]->_visible = true;
   _layers["players-name"]->_visible = false;
   _layers["temp_bg"]->_visible = true;
   _layers["title"]->_visible = true;
   _layers["window"]->_visible = true;

   _layers["name-error-msg"]->_visible = false;
   _layers["Please enter your name"]->_visible = true;

   _layers["delete_xbox_0"]->_visible = isControllerUsed();
   _layers["delete_xbox_1"]->_visible = false;
   _layers["delete_pc_0"]->_visible = !isControllerUsed();
   _layers["delete_pc_1"]->_visible = false;

   _layers["confirm_xbox_0"]->_visible = isControllerUsed();
   _layers["confirm_xbox_1"]->_visible = false;
   _layers["confirm_pc_0"]->_visible = !isControllerUsed();
   _layers["confirm_pc_1"]->_visible = false;

   _layers["cancel_xbox_0"]->_visible = isControllerUsed();
   _layers["cancel_xbox_1"]->_visible = false;
   _layers["cancel_pc_0"]->_visible = !isControllerUsed();
   _layers["cancel_pc_1"]->_visible = false;
}


void MenuScreenNameSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);
   window.draw(_text, states);
}


