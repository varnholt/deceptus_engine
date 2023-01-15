#include "menuscreenfileselect.h"

#include "menu.h"
#include "menuaudio.h"

#include "game/gamestate.h"
#include "game/messagebox.h"
#include "game/savestate.h"

#include <iostream>
#include <ostream>
#include <sstream>


namespace {
   int32_t nameOffsetY = -2;
}


MenuScreenFileSelect::MenuScreenFileSelect()
{
   setFilename("data/menus/fileselect.psd");

   _font.loadFromFile("data/fonts/deceptum.ttf");
   const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);

   for (auto i = 0u; i < 3; i++)
   {
      _names[i].setFont(_font);
      _names[i].setCharacterSize(12);
      _names[i].setFillColor(sf::Color{232, 219, 243});
   }
}


void MenuScreenFileSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   for (auto i = 0u; i < 3; i++)
   {
      window.draw(_names[i], states);
   }
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
      "Are you sure you want to delete this file?",
      [this](MessageBox::Button button) {
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

   else if (key == sf::Keyboard::Delete)
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
      _layers["slot_" + slot_name + "_new_game_highlight"]->_visible = empty;
      _layers["slot_" + slot_name + "_new_game_deselected"]->_visible = empty && !selected;
      _layers["slot_" + slot_name + "_new_game_selected"]->_visible = empty && selected;
      _layers["slot_" + slot_name + "_shadow"]->_visible = empty;

      // have data
      _layers["slot_" + slot_name + "_selected"]->_visible = !empty && selected;
      _layers["slot_" + slot_name + "_deselected"]->_visible = !empty && !selected;
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
      _names[index].setString(save_state._player_info._name);
      _names[index].setPosition(
         layer_name->_sprite->getPosition().x,
         layer_name->_sprite->getPosition().y + nameOffsetY
      );

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
}


