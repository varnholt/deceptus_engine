#pragma once

#include <vector>
#include <string>

#include <SFML/Graphics.hpp>

#include "constants.h"

struct TmxObject;

class Dialogue
{

public:

   struct DialogueItem
   {
      std::string mTitle;
      std::string mMessage;
      sf::Color mTextColor = sf::Color{232, 219, 243};
      sf::Color mBackgroundColor = sf::Color{47, 12, 75};
      MessageBoxLocation mLocation = MessageBoxLocation::MiddleCenter;
   };

   Dialogue() = default;
   static void add(TmxObject* tmxObject);
   static void resetAll();
   static void update();

   bool isActive() const;
   void setActive(bool active);
   void showNext();

private:

   std::vector<DialogueItem> mDialogue;
   uint32_t mIndex = 0;

   sf::IntRect mPixelRect;
   bool mRepeated = false;
   bool mPlayed = false;
   bool mActive = false;

   static std::vector<Dialogue> sDialogues;
};


