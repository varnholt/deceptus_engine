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
   void left();
   void right();

   void select();
   void back();

private:

   std::string mName;
   std::array<unsigned char, 13 * 5> mChars;

   sf::Vector2i mCharOrigin;
   sf::Vector2i mCharOffset;
};

