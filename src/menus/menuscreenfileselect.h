#pragma once

#include <array>
#include "menuscreen.h"

class MenuScreenFileSelect : public MenuScreen
{
public:
   enum class Slot
   {
      A = 0,
      B = 1,
      C = 2
   };

   MenuScreenFileSelect();

   void draw(sf::RenderTarget& window, sf::RenderStates states) override;
   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void controllerButtonX() override;
   void loadingFinished() override;

   void showEvent() override;

private:
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void remove();

   sf::Font _font;
   std::array<std::unique_ptr<sf::Text>, 3> _names;

   Slot _slot = Slot::A;
};
