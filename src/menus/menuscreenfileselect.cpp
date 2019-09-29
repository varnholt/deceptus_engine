#include "menuscreenfileselect.h"

#include "menu.h"

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

   mFont.loadFromFile("data/fonts/deceptum.ttf");

   for (auto i = 0u; i < 3; i++)
   {
      mNames[i].setScale(0.25f, 0.25f);
      mNames[i].setFont(mFont);
      mNames[i].setCharacterSize(48);
      mNames[i].setFillColor(sf::Color{232, 219, 243});
   }
}


void MenuScreenFileSelect::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   for (auto i = 0u; i < 3; i++)
   {
      window.draw(mNames[i], states);
   }
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
   SaveState::setCurrent(static_cast<uint32_t>(mSlot));

   auto& saveState = SaveState::getCurrent();
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
   }
}


void MenuScreenFileSelect::back()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
}


void MenuScreenFileSelect::remove()
{
   MessageBox::question(
      "Are you sure you want to delete this file?",
      [this](MessageBox::Button button) {
         if (button == MessageBox::Button::Yes)
         {
            SaveState::getSaveState(static_cast<uint32_t>(mSlot)).invalidate();
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


void MenuScreenFileSelect::loadingFinished()
{
   SaveState::deserializeFromFile();
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
      mLayers["slot_" + slotName + "_shadow"]->mVisible = empty;

      // have data
      mLayers["slot_" + slotName + "_selected"]->mVisible = !empty && selected;
      mLayers["slot_" + slotName + "_deselected"]->mVisible = !empty && !selected;
      mLayers["slot_" + slotName + "_background"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_bar_1"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_bar_2"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_energy"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_heart"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_highlight"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_letter_deselected"]->mVisible = !selected;
      mLayers["slot_" + slotName + "_letter_selected"]->mVisible = selected;
      mLayers["slot_" + slotName + "_lines"]->mVisible = !empty;
      mLayers["slot_" + slotName + "_name"]->mVisible = false;
      mLayers["slot_" + slotName + "_progress"]->mVisible = false;
      mLayers["slot_" + slotName + "_time"]->mVisible = false;

      // both
      mLayers["slot_" + slotName + "_arrow"]->mVisible = selected;

      // update names
      auto nameLayer = mLayers["slot_" + slotName + "_name"];
      mNames[index].setString(saveState.mPlayerInfo.mName);
      mNames[index].setPosition(
         nameLayer->mSprite->getPosition().x,
         nameLayer->mSprite->getPosition().y + nameOffsetY
      );

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


