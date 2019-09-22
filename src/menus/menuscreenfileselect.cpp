#include "menuscreenfileselect.h"

#include "menu.h"

#include "game/gamestate.h"
#include "game/savestate.h"

#include <iostream>
#include <ostream>
#include <sstream>


MenuScreenFileSelect::MenuScreenFileSelect()
{
   setFilename("data/menus/fileselect.psd");
}


void MenuScreenFileSelect::up()
{
   auto idx = static_cast<int32_t>(mSlot);
   idx--;
   if (idx < 0)
   {
      idx = 0;
   }
   mSlot = static_cast<Slot>(idx);
   updateLayers();
}


void MenuScreenFileSelect::down()
{
   auto idx = static_cast<int32_t>(mSlot);
   idx++;
   if (idx > 2)
   {
      idx = 2;
   }
   mSlot = static_cast<Slot>(idx);
   updateLayers();
}


void MenuScreenFileSelect::select()
{
   const auto newGame = true;

   if (newGame)
   {
      Menu::getInstance()->show(Menu::MenuType::NameSelect);
   }
   else
   {
      Menu::getInstance()->hide();
      GameState::getInstance().enqueueResume();
   }

   // if current slot holds data, load it

   // if current slot is empty, create a new slot and go to name select
}


void MenuScreenFileSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
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
   // for (auto& layer : mLayers)
   // {
   //    std::cout << layer.first << std::endl;
   // }

   auto index = 0;

   const auto& saveStates = SaveState::getSaveStates();
   for (const auto& saveState : saveStates)
   {
      std::ostringstream out;
      out << (index + 1);

      const auto empty = saveState.isEmpty();
      const auto slotName = out.str();
      const auto selected = index == static_cast<int32_t>(mSlot);

      // no data
      mLayers["slot_" + slotName + "_new_game"]->mVisible = empty;
      mLayers["slot_" + slotName + "_new_game_background"]->mVisible = empty;
      mLayers["slot_" + slotName + "_new_game_highlight"]->mVisible = empty;
      mLayers["slot_" + slotName + "_new_game_deselected"]->mVisible = empty && !selected;
      mLayers["slot_" + slotName + "_new_game_selected"]->mVisible = empty && selected;
      mLayers["slot_" + slotName + "_selected"]->mVisible = empty && selected;
      mLayers["slot_" + slotName + "_shadow"]->mVisible = empty;

      // have data
      mLayers["slot_" + slotName + "_background"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_bar_1"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_bar_2"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_energy"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_heart"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_highlight"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_letter_deselected"]->mVisible = !empty && !selected;
      mLayers["slot_" + slotName + "_letter_selected"]->mVisible = !empty && selected;
      mLayers["slot_" + slotName + "_lines"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_name"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_progress"]->mVisible = false;
      mLayers["slot_" + slotName + "_time"]->mVisible = false;

      // both
      mLayers["slot_" + slotName + "_arrow"]->mVisible = selected;

      index++;
   }

   mLayers["delete_xbox_0"]->mVisible = isControllerUsed();
   mLayers["delete_xbox_1"]->mVisible = false;
   mLayers["delete_pc_0"]->mVisible = !isControllerUsed();
   mLayers["delete_pc_1"]->mVisible = false;

   mLayers["confirm_xbox_0"]->mVisible = isControllerUsed();
   mLayers["confirm_xbox_1"]->mVisible = false;
   mLayers["confirm_pc_0"]->mVisible = !isControllerUsed();
   mLayers["confirm_pc_1"]->mVisible = false;

   mLayers["back_xbox_0"]->mVisible = isControllerUsed();
   mLayers["back_xbox_1"]->mVisible = false;
   mLayers["back_pc_0"]->mVisible = !isControllerUsed();
   mLayers["back_pc_1"]->mVisible = false;
}


