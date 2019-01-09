#pragma once

#include "menuscreen.h"

class MenuScreenControls : public MenuScreen
{
public:
   MenuScreenControls();

   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();
};
