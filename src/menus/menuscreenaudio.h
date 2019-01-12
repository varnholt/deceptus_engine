#pragma once

#include "menuscreen.h"

#include <cstdint>


class MenuScreenAudio : public MenuScreen
{
public:

   enum class Selection {
      Master = 0,
      Music  = 1,
      SFX    = 2,
      Count  = 3,
   };

   MenuScreenAudio();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void set(int32_t x);

   Selection mSelection = Selection::Master;
};
