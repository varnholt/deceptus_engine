#include "dialogue.h"

#include "gamestate.h"
#include "messagebox.h"
#include "player/player.h"
#include "savestate.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

std::vector<Dialogue> Dialogue::sDialogues;


void Dialogue::add(TmxObject* tmxObject)
{
   Dialogue dialogue;

   auto properties = tmxObject->_properties;
   for (auto i = 0u; i < 99; i++)
   {
      std::ostringstream oss;
      oss << std::setw(2) << std::setfill('0') << i;

      auto it = properties->_map.find(oss.str());
      if (it != properties->_map.end())
      {
         DialogueItem item;
         item.mMessage = (*it).second->_value_string.value();
         dialogue.mDialogue.push_back(item);
      }
   }

   dialogue.mPixelRect = sf::IntRect{
      static_cast<int32_t>(tmxObject->_x_px),
      static_cast<int32_t>(tmxObject->_y_px),
      static_cast<int32_t>(tmxObject->_width_px),
      static_cast<int32_t>(tmxObject->_height_px)
   };

   sDialogues.push_back(dialogue);
}


void Dialogue::resetAll()
{
   sDialogues.clear();
}


void Dialogue::update()
{
   // prevent 'pause visible' vs. 'dialogue visible' if both
   // are activated in the same frame.
   if (
         GameState::getInstance().getMode() == ExecutionMode::Paused
      || GameState::getInstance().getQueuedMode() == ExecutionMode::Paused
   )
   {
      return;
   }

   // check whether up button is pressed
   // actually there could be a number of 'message box activation buttons' here but for
   // now it might be sufficient to just check for the up button
   if (!Player::getCurrent()->getControls().isUpButtonPressed())
   {
      return;
   }

   auto playerRect = Player::getCurrent()->getPlayerPixelRect();

   for (auto& dialogue : sDialogues)
   {
      if (playerRect.intersects(dialogue.mPixelRect))
      {
         if (!dialogue.isActive())
         {
            dialogue.setActive(true);
            dialogue.showNext();
         }
      }
      else
      {
         dialogue.setActive(false);
      }
   }
}


bool Dialogue::isActive() const
{
   return mActive;
}


void Dialogue::setActive(bool active)
{
   mActive = active;
}


void Dialogue::replace(std::string& str, const std::string& what, const std::string& with)
{
   auto index = str.find(what, 0);
   if (index != std::string::npos)
   {
      str.replace(index, what.size(), with);
   }
}


void Dialogue::replaceTags(std::string& str)
{
   replace(str, "<player>", SaveState::getPlayerInfo().mName);
   replace(str, "<br>", "\n");
}


void Dialogue::showNext()
{
   if (mIndex == mDialogue.size())
   {
      mIndex = 0;

      // when done, mark the dialogue as inactive so it can be reactivated on button press
      setActive(false);
      return;
   }

   const auto item = mDialogue.at(mIndex);

   auto str = item.mMessage;

   replaceTags(str);

   MessageBox::info(
      str,
      [this](MessageBox::Button /*b*/) {
         showNext();
      },
      MessageBox::LayoutProperties{item.mLocation, item.mBackgroundColor, item.mTextColor, true, false}
   );

   mIndex++;
}



