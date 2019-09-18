#pragma once

#include "menuscreen.h"

class MenuScreenNameSelect : public MenuScreen
{
public:
   MenuScreenNameSelect();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();
};

