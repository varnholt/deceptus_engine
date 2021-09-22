#pragma once

#include "menuscreen.h"

class MenuScreenGame : public MenuScreen
{
public:

   enum class Selection {
      TextSpeed = 0,
      AutomaticPause = 1,
      Count = 2,
   };

   MenuScreenGame();

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   Selection _selection = Selection::TextSpeed;
};

