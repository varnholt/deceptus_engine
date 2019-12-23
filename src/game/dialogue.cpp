#include "dialogue.h"

#include "messagebox.h"
#include "player.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"

#include <iomanip>
#include <iostream>
#include <sstream>

std::vector<Dialogue> Dialogue::sDialogues;


void Dialogue::add(TmxObject* tmxObject)
{
   Dialogue dialogue;

   auto properties = tmxObject->mProperties;
   for (auto i = 0u; i < 99; i++)
   {
      std::ostringstream oss;
      oss << std::setw(2) << std::setfill('0') << i;

      auto it = properties->mMap.find(oss.str());
      if (it != properties->mMap.end())
      {
         DialogueItem item;
         item.mMessage = (*it).second->mValueStr;
         dialogue.mDialogue.push_back(item);
      }
   }

   dialogue.mPixelRect = sf::IntRect{
      static_cast<int32_t>(tmxObject->mX),
      static_cast<int32_t>(tmxObject->mY),
      static_cast<int32_t>(tmxObject->mWidth),
      static_cast<int32_t>(tmxObject->mHeight)
   };

   sDialogues.push_back(dialogue);
}


void Dialogue::resetAll()
{
   sDialogues.clear();
}


void Dialogue::update()
{
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


void Dialogue::showNext()
{
   if (mIndex == mDialogue.size())
   {
      mIndex = 0;
      return;
   }

   MessageBox::info(
      mDialogue.at(mIndex).mMessage,
      [this](MessageBox::Button /*b*/) {
         showNext();
      }
   );

   mIndex++;
}

