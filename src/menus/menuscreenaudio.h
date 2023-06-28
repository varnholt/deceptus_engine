#pragma once

#include "menuscreen.h"

#include <cstdint>
#include <vector>

class MenuScreenAudio : public MenuScreen
{
public:
   enum class Selection
   {
      Master = 0,
      Music = 1,
      SFX = 2,
      Count = 3,
   };

   MenuScreenAudio();
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   void loadingFinished() override;
   void showEvent() override;

   void updateLayers();

   void up();
   void down();
   void select();
   void back();

   void set(int32_t x);
   void setDefaults();

   Selection _selection = Selection::Master;

   std::vector<std::shared_ptr<Layer>> _volume_layers_master;
   std::vector<std::shared_ptr<Layer>> _volume_layers_music;
   std::vector<std::shared_ptr<Layer>> _volume_layers_sfx;
};
