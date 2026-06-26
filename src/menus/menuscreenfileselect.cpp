#include "menuscreenfileselect.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "menu.h"
#include "menuaudio.h"

#include "game/level/leveldescription.h"
#include "game/level/levels.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/ui/messagebox.h"

#include <iostream>
#include <ostream>
#include <sstream>

namespace
{
int32_t nameOffsetY = -2;
}

MenuScreenFileSelect::MenuScreenFileSelect()
{
   setFilename("data/menus/fileselect.psd");

   for (auto i = 0u; i < 3; i++)
   {
      _names[i] = std::make_unique<sf::Text>(_font);
      _names[i]->setCharacterSize(12);
      _names[i]->setFillColor(sf::Color{232, 219, 243});
   }

   for (auto slot_index = 0u; slot_index < 3; slot_index++)
   {
      _new_game_texts[slot_index] = std::make_unique<sf::Text>(_font);
      _new_game_texts[slot_index]->setCharacterSize(12);
      _new_game_texts[slot_index]->setFillColor(color_label_normal);
   }

   _text_back_button = std::make_unique<sf::Text>(_font);
   _text_back_button->setCharacterSize(12);
   _text_back_button->setFillColor(color_label_normal);
   _text_delete_button = std::make_unique<sf::Text>(_font);
   _text_delete_button->setCharacterSize(12);
   _text_delete_button->setFillColor(color_label_normal);
   _text_confirm_button = std::make_unique<sf::Text>(_font);
   _text_confirm_button->setCharacterSize(12);
   _text_confirm_button->setFillColor(color_label_normal);
}

void MenuScreenFileSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   for (auto i = 0u; i < 3; i++)
   {
      window.draw(*_names[i], states);
   }

   for (auto slot_index = 0u; slot_index < 3; slot_index++)
   {
      window.draw(*_new_game_texts[slot_index], states);
   }

   if (!_text_back_button)
   {
      return;
   }
   window.draw(*_text_back_button, states);
   window.draw(*_text_delete_button, states);
   window.draw(*_text_confirm_button, states);
}

void MenuScreenFileSelect::up()
{
   auto idx = static_cast<int32_t>(_slot);
   idx--;
   if (idx < 0)
   {
      idx = 0;
   }
   _slot = static_cast<Slot>(idx);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenFileSelect::down()
{
   auto idx = static_cast<int32_t>(_slot);
   idx++;
   if (idx > 2)
   {
      idx = 2;
   }
   _slot = static_cast<Slot>(idx);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenFileSelect::select()
{
   SaveState::setCurrent(static_cast<uint32_t>(_slot));

   const auto& saveState = SaveState::getCurrent();
   if (saveState.isEmpty())
   {
      // if current slot is empty, create a new slot and go to name select
      Menu::getInstance()->show(Menu::MenuType::NameSelect);
   }
   else
   {
      // do a quick sanity check if the level is actually in place, otherwise throw an error
      const auto level_item = Levels::readLevelItem(SaveState::getCurrent()._level_index);
      auto description = LevelDescription::load(level_item._level_name);
      if (!description)
      {
         MessageBox::info(tr("The selected level is not available."), [](MessageBox::Button) {});
         return;
      }

      // if current slot holds data, load it
      Menu::getInstance()->hide();
      GameState::getInstance().enqueueResume();

      // request level-reloading since we updated the save state
      SaveState::getCurrent()._load_level_requested = true;
   }

   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenFileSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenFileSelect::remove()
{
   MessageBox::question(
      tr("Are you sure you want to delete this file?"),
      [this](MessageBox::Button button)
      {
         if (button == MessageBox::Button::Yes)
         {
            SaveState::getSaveState(static_cast<uint32_t>(_slot)).invalidate();
            SaveState::serializeToFile();
            updateLayers();
         }
      }
   );
}

void MenuScreenFileSelect::keyboardKeyPressed(sf::Keyboard::Key key)
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

   else if (key == sf::Keyboard::Key::Delete)
   {
      remove();
   }
}

void MenuScreenFileSelect::controllerButtonX()
{
   remove();
}

void MenuScreenFileSelect::loadingFinished()
{
   SaveState::deserializeFromFile();
   updateLayers();
}

void MenuScreenFileSelect::showEvent()
{
   // always update to latest save state when the menu is shown.
   // when a new game is started, the save state changes, so the
   // layers become outdated.
   updateLayers();
}

void MenuScreenFileSelect::updateLayers()
{
   auto index = 0;

   const auto& save_states = SaveState::getSaveStates();
   for (const auto& save_state : save_states)
   {
      std::ostringstream out;
      out << (index + 1);

      const auto empty = save_state.isEmpty();
      const auto slot_name = out.str();
      const auto selected = index == static_cast<int32_t>(_slot);

      // no data
      _layers["slot_" + slot_name + "_new_game"]->_visible = empty;
      _layers["slot_" + slot_name + "_new_game_background"]->_visible = empty;
      _layers["slot_" + slot_name + "_new_game_highlight"]->_visible = empty && selected;
      _layers["slot_" + slot_name + "_new_game_deselected"]->_visible = empty && !selected;
      _layers["slot_" + slot_name + "_new_game_selected"]->_visible = empty && selected;

      if (empty)
      {
         _new_game_texts[index]->setString(sftr("New Game"));
         _new_game_texts[index]->setFillColor(selected ? color_label_selected : color_label_normal);
         placeTextCentered(*_new_game_texts[index], _layers["slot_" + slot_name + "_new_game_background"]->_sprite->getGlobalBounds());
      }
      else
      {
         _new_game_texts[index]->setString({});
      }
      _layers["slot_" + slot_name + "_shadow"]->_visible = empty;

      // have data
      _layers["slot_" + slot_name + "_selected"]->_visible = selected;
      _layers["slot_" + slot_name + "_deselected"]->_visible = !selected;
      _layers["slot_" + slot_name + "_background"]->_visible = !empty;
      _layers["slot_" + slot_name + "_bar_1"]->_visible = !empty;
      _layers["slot_" + slot_name + "_bar_2"]->_visible = !empty;
      _layers["slot_" + slot_name + "_energy"]->_visible = !empty;
      _layers["slot_" + slot_name + "_heart"]->_visible = !empty;
      _layers["slot_" + slot_name + "_highlight"]->_visible = !empty;
      _layers["slot_" + slot_name + "_letter_deselected"]->_visible = !selected;
      _layers["slot_" + slot_name + "_letter_selected"]->_visible = selected;
      _layers["slot_" + slot_name + "_lines"]->_visible = !empty;
      _layers["slot_" + slot_name + "_name"]->_visible = false;
      _layers["slot_" + slot_name + "_progress"]->_visible = false;
      _layers["slot_" + slot_name + "_time"]->_visible = false;

      // both
      _layers["slot_" + slot_name + "_arrow"]->_visible = selected;

      // update names
      auto layer_name = _layers["slot_" + slot_name + "_name"];
      _names[index]->setString(save_state._player_info._name);
      _names[index]->position = {layer_name->_sprite->position.x, layer_name->_sprite->position.y + nameOffsetY};

      index++;
   }

   _layers["delete_xbox_0"]->_visible = isControllerUsed();
   _layers["delete_xbox_1"]->_visible = false;
   _layers["delete_pc_0"]->_visible = !isControllerUsed();
   _layers["delete_pc_1"]->_visible = false;

   _layers["confirm_xbox_0"]->_visible = isControllerUsed();
   _layers["confirm_xbox_1"]->_visible = false;
   _layers["confirm_pc_0"]->_visible = !isControllerUsed();
   _layers["confirm_pc_1"]->_visible = false;

   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   if (!_text_back_button)
   {
      return;
   }

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& delete_layer = isControllerUsed() ? _layers["delete_xbox_0"] : _layers["delete_pc_0"];
   _text_delete_button->setString(sftr("Delete"));
   placeTextRightOf(*_text_delete_button, delete_layer->_sprite->getGlobalBounds());

   const auto& confirm_layer = isControllerUsed() ? _layers["confirm_xbox_0"] : _layers["confirm_pc_0"];
   _text_confirm_button->setString(sftr("Confirm"));
   placeTextRightOf(*_text_confirm_button, confirm_layer->_sprite->getGlobalBounds());
}
