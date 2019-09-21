#pragma once

#include "menuscreen.h"

class MenuScreenFileSelect : public MenuScreen
{
public:

   enum class Slot{
      A = 0,
      B = 1,
      C = 2
   };

   MenuScreenFileSelect();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

private:


   Slot mSlot = Slot::A;
};

