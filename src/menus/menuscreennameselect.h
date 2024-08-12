#pragma once

#include "menuscreen.h"

#include <array>

class MenuScreenNameSelect : public MenuScreen
{
public:
   MenuScreenNameSelect();

   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void keyboardKeyReleased(sf::Keyboard::Key key) override;

   void controllerButtonX() override;
   void controllerButtonY() override;

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
   void chop();
   void appendChar(char enteredChar);
   void updateText();
   void retrieveUsername();

   std::string _name;
   std::array<char, 13 * 5> _chars;

   sf::Vector2f _char_origin;
   sf::Vector2i _char_offset;

   sf::Rect<float> _name_rect;

   sf::Font _font;
   sf::Text _text;

   int32_t _shift = 0;
};
