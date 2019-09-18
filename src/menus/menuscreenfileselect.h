#pragma once

#include "menuscreen.h"

class MenuScreenFileSelect : public MenuScreen
{
public:
   MenuScreenFileSelect();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();
};

