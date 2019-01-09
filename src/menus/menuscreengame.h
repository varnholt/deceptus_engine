#pragma once

#include "menuscreen.h"

class MenuScreenGame : public MenuScreen
{
public:

   enum class Selection {
      Language = 0,
      TextSpeed = 1,
      AutomaticPause = 2,
      Count = 3,
   };

   MenuScreenGame();

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   Selection mSelection = Selection::Language;
};

