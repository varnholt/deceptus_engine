#pragma once

#include "menuscreen.h"

class MenuScreenOptions : public MenuScreen
{
public:
   MenuScreenOptions();

   enum class Selection {
      Controls = 0,
      Video = 1,
      Audio = 2,
      Game = 3,
      Achievements = 4,
      Credits = 5,
      Count
   };

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void updateLayers();
   void loadingFinished() override;

   void up();
   void down();
   void back();
   void select();

   Selection _selection = Selection::Controls;
};

