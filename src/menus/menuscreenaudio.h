#pragma once

#include "menuscreen.h"

class MenuScreenAudio : public MenuScreen
{
public:

   enum class Selection {
      Master = 0,
      Music  = 1,
      SFX    = 2,
      Count  = 3,
   };

   MenuScreenAudio();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   Selection mSelection = Selection::Master;
};
