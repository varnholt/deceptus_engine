#pragma once

#include <vector>
#include <string>

#include <sfml/Graphics.hpp>

struct TmxObject;

class Dialogue
{

public:

   enum class Location
   {
      Invalid,
      TopLeft,
      TopCenter,
      TopRight,
      MiddleLeft,
      MiddleCenter,
      MiddleRight,
      BottomLeft,
      BottomCenter,
      BottomRight,
   };

   struct DialogueItem
   {
      std::string mName;
      std::string mMessage;
      sf::Color mColor;
      Location mLocation;
   };

   Dialogue() = default;
   static void add(TmxObject* tmxObject);
   static void resetAll();
   static void update();

private:
   std::vector<DialogueItem> mDialogue;
   sf::IntRect mPixelRect;
   bool mRepeated = false;
   bool mPlayed = false;
   bool mDisplayed = false;

   static std::vector<Dialogue> sDialogues;
};


