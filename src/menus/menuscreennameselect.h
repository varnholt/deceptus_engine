#pragma once

#include "menuscreen.h"

#include <array>

class MenuScreenNameSelect : public MenuScreen
{
public:
   MenuScreenNameSelect();

   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void keyboardKeyReleased(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   void up();
   void down();
   void left();
   void right();

   void select();
   void back();

private:

   void updateText();
   void retrieveUsername();

   std::string mName;
   std::array<char, 13 * 5> mChars;

   sf::Vector2f mCharOrigin;
   sf::Vector2i mCharOffset;

   sf::Rect<float> mNameRect;

   sf::Font mFont;
   sf::Text mText;

   int32_t mShift = 0;
};

