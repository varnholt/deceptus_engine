#pragma once

#include "menuscreen.h"

#include <array>

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

private:

   std::array<unsigned char, 13 * 5> mChars;
   uint8_t mX = 0;
   uint8_t mY = 0;
};

