#pragma once

#include "menuscreen.h"

#include <functional>

class MenuScreenMain : public MenuScreen
{
public:
   enum class Selection
   {
      Start,
      Options,
      Quit
   };

   using ExitCallback = std::function<void(void)>;

   MenuScreenMain();

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();

   void setExitCallback(ExitCallback callback);

   Selection _selection = Selection::Start;

private:
   ExitCallback _exit_callback;

   sf::Font _font;
   sf::Text _text_build;
   sf::Text _text_year;
};
