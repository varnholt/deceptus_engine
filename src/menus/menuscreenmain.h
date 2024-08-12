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
};
