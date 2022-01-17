#pragma once

#include "menuscreen.h"

class MenuScreenGame : public MenuScreen
{
public:

   enum class Selection {
      TextSpeed = 0,
      AutomaticPause = 1,
      Count = 2,
   };

   MenuScreenGame();


protected:

   void keyboardKeyPressed(sf::Keyboard::Key key) override;
   void loadingFinished() override;


private:

   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void set(int32_t x);

   Selection _selection = Selection::TextSpeed;
};

