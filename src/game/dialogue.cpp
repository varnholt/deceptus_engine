#include "dialogue.h"

#include "messagebox.h"
#include "player.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"

#include <iostream>

std::vector<Dialogue> Dialogue::sDialogues;


void Dialogue::add(TmxObject* tmxObject)
{

   std::cout << "add text" << std::endl;

   Dialogue dialogue;

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
           if (!dialogue.mDisplayed)
           {
              dialogue.mDisplayed = true;
              std::cout << "show text" << std::endl;
              MessageBox::info("Yo, yo!", nullptr);
           }
       }
   }
}

