#pragma once

#include "menuscreen.h"

class MenuScreenVideo : public MenuScreen
{
public:

   enum class Selection {
      Monitor     = 0,
      Resolution  = 1,
      DisplayMode = 2,
      VSync       = 3,
      Brightness  = 4,
      Count       = 5
   };

   MenuScreenVideo();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   Selection mSelection = Selection::Monitor;
};

