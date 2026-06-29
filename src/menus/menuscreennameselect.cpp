#include "menuscreennameselect.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "menu.h"

#include <cstdlib>
#include <iostream>

namespace
{
static const int32_t char_width = 19;
static const int32_t char_height = 24;
static const size_t max_length = 11;

std::string extractFirstName(std::string_view username)
{
   // heuristic 1: split CamelCase
   for (std::size_t i = 1; i < username.size(); ++i)
   {
      if (std::isupper(static_cast<unsigned char>(username[i])))
      {
         return std::string(username.substr(0, i));
      }
   }

   // heuristic 2: try underscores or dots
   if (auto pos = username.find_first_of("._"); pos != std::string_view::npos)
   {
      return std::string(username.substr(0, pos));
   }

   // fallback: return full username
   return std::string(username);
}
}  // namespace

MenuScreenNameSelect::MenuScreenNameSelect()
{
   _text = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text->setFont(_font);
   _text->setCharacterSize(12);
   _text->setFillColor(sf::Color{232, 219, 243});

   setFilename("data/menus/nameselect.psd");

   _chars = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
      'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
      's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '.', '-',
   };

   _text_cancel_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_cancel_button->setCharacterSize(12);
   _text_cancel_button->setFillColor(color_label_normal);
   _text_delete_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_delete_button->setCharacterSize(12);
   _text_delete_button->setFillColor(color_label_normal);
   _text_confirm_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_confirm_button->setCharacterSize(12);
   _text_confirm_button->setFillColor(color_label_normal);
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
   _text->setString(_name.c_str());
   const auto text_rect = _text->getLocalBounds();
   const auto x_offset_px = (_name_rect.size.x - text_rect.size.x) * 0.5f;
   const auto x_px = _name_rect.position.x + x_offset_px;
   _text->position = {x_px, _name_rect.position.y};
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
   _shift += static_cast<int32_t>(key == sf::Keyboard::Key::LShift);
   _shift += static_cast<int32_t>(key == sf::Keyboard::Key::RShift);

   if (key >= sf::Keyboard::Key::A && key <= sf::Keyboard::Key::Z)
   {
      if (_name.size() >= max_length)
      {
         return;
      }

      const auto c = _chars[static_cast<uint32_t>(key) + (_shift ? 0 : 26)];
      appendChar(c);
   }

   if (key == sf::Keyboard::Key::Delete || key == sf::Keyboard::Key::Backspace)
   {
      chop();
   }

   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }

   if (key == sf::Keyboard::Key::Left)
   {
      left();
   }

   else if (key == sf::Keyboard::Key::Right)
   {
      right();
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

void MenuScreenNameSelect::keyboardKeyReleased(sf::Keyboard::Key key)
{
   _shift -= static_cast<int32_t>(key == sf::Keyboard::Key::LShift);
   _shift -= static_cast<int32_t>(key == sf::Keyboard::Key::RShift);
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
   auto* u1 = std::getenv("USERNAME");
   auto* u2 = std::getenv("USER");
   std::string raw_name = u1 ? u1 : (u2 ? u2 : "");
   _name = extractFirstName(raw_name);

   if (!_name.empty())
   {
      _name[0] = static_cast<char>(std::toupper(_name[0]));
      updateText();
   }
}

void MenuScreenNameSelect::loadingFinished()
{
   const auto cursor = _layers["cursor"];
   _char_origin.x = cursor->_sprite->position.x;
   _char_origin.y = cursor->_sprite->position.y;

   const auto player_name = _layers["players-name"];
   _name_rect.position.x = player_name->_sprite->position.x;
   _name_rect.position.y = player_name->_sprite->position.y;
   _name_rect.size.x = static_cast<float>(player_name->_texture->getSize().x);

   retrieveUsername();
   updateLayers();
}

void MenuScreenNameSelect::updateLayers()
{
   auto cursor = _layers["cursor"];
   cursor->_sprite->position = {
      static_cast<float>(_char_origin.x + _char_offset.x * char_width), static_cast<float>(_char_origin.y + _char_offset.y * char_height)
   };

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

   if (!_text_cancel_button)
   {
      return;
   }

   const auto& cancel_layer = isControllerUsed() ? _layers["cancel_xbox_0"] : _layers["cancel_pc_0"];
   _text_cancel_button->setString(sftr("Cancel"));
   placeTextRightOf(*_text_cancel_button, cancel_layer->_sprite->getGlobalBounds());

   const auto& delete_layer = isControllerUsed() ? _layers["delete_xbox_0"] : _layers["delete_pc_0"];
   _text_delete_button->setString(sftr("Delete"));
   placeTextRightOf(*_text_delete_button, delete_layer->_sprite->getGlobalBounds());

   const auto& confirm_layer = isControllerUsed() ? _layers["confirm_xbox_0"] : _layers["confirm_pc_0"];
   _text_confirm_button->setString(sftr("Confirm"));
   placeTextRightOf(*_text_confirm_button, confirm_layer->_sprite->getGlobalBounds());
}

void MenuScreenNameSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);
   window.draw(*_text, states);

   if (!_text_cancel_button)
   {
      return;
   }
   window.draw(*_text_cancel_button, states);
   window.draw(*_text_delete_button, states);
   window.draw(*_text_confirm_button, states);
}
