#pragma once

#include "menuscreen.h"

class MenuScreenGame : public MenuScreen
{
public:
   enum class Selection
   {
      TextSpeed = 0,
      Rumble = 1,
      AutomaticPause = 2,
      Count = 3,
   };

   MenuScreenGame();

protected:
   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void loadingFinished() override;

private:
   bool isRumbleEnabled() const;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void set(int32_t x);

   Selection _selection = Selection::TextSpeed;
};
