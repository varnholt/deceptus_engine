#pragma once

#include "menuscreen.h"

class MenuScreenMain : public MenuScreen
{

public:

   enum class Selection {
      Start,
      Options,
      Quit
   };

   MenuScreenMain();

   void update(float dt) override;

   Selection mSelection = Selection::Start;

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();

};

