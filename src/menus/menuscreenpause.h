#pragma once

#include "menuscreen.h"

#include <functional>

class MenuScreenPause : public MenuScreen
{
public:
   enum class Selection
   {
      Resume,
      Options,
      Quit
   };

   MenuScreenPause();

   void update(const sf::Time& dt) override;

   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();

   void showEvent() override;

   Selection _selection = Selection::Resume;

private:
   void resume();
};
