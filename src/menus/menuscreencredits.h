#pragma once

#include "menuscreen.h"

class MenuScreenCredits : public MenuScreen
{
public:
   MenuScreenCredits();

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;
};

