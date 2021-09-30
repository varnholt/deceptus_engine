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


std::shared_ptr<Dialogue> Dialogue::deserialize(TmxObject* tmxObject)
{
   auto dialogue = std::make_shared<Dialogue>();

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
         dialogue->_dialogue_items.push_back(item);
      }
   }

   dialogue->_pixel_rect = sf::IntRect{
      static_cast<int32_t>(tmxObject->_x_px),
      static_cast<int32_t>(tmxObject->_y_px),
      static_cast<int32_t>(tmxObject->_width_px),
      static_cast<int32_t>(tmxObject->_height_px)
   };

   return dialogue;
}


void Dialogue::update(const sf::Time& /*dt*/)
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

   if (playerRect.intersects(_pixel_rect))
   {
      if (!isActive())
      {
         setActive(true);
         showNext();
      }
   }
   else
   {
      setActive(false);
   }
}


bool Dialogue::isActive() const
{
   return _active;
}


void Dialogue::setActive(bool active)
{
   _active = active;
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
   if (_index == _dialogue_items.size())
   {
      _index = 0;

      // when done, mark the dialogue as inactive so it can be reactivated on button press
      setActive(false);
      return;
   }

   const auto item = _dialogue_items.at(_index);

   auto str = item.mMessage;

   replaceTags(str);

   MessageBox::info(
      str,
      [this](MessageBox::Button /*b*/) {showNext();},
      MessageBox::LayoutProperties{
         item.mLocation,
         item.mBackgroundColor,
         item.mTextColor,
         true,
         false,
         (_index == 0)
      }
   );

   _index++;
}



