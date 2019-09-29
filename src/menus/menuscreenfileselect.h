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

   void draw(sf::RenderTarget& window, sf::RenderStates states) override;
   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void controllerButtonX() override;
   void loadingFinished() override;


private:

   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void remove();

   sf::Font mFont;
   sf::Text mNames[3];

   Slot mSlot = Slot::A;
};

