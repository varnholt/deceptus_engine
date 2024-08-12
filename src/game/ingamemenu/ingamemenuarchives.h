#ifndef INGAMEMENUARCHIVES_H
#define INGAMEMENUARCHIVES_H

#include "game/image/layerdata.h"
#include "ingamemenupage.h"

#include <SFML/Graphics.hpp>

class InGameMenuArchives : public InGameMenuPage
{
public:
   InGameMenuArchives();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
   void update(const sf::Time& dt) override;

   void up() override;
   void down() override;

private:
   void updateMove();
   void updateShowHide();
   void updateButtons();

   std::vector<LayerData> _panel_header;
   std::vector<LayerData> _panel_left;
   std::vector<LayerData> _panel_right;
   std::vector<LayerData> _panel_background;

   int32_t _selected_index = 0;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
};

#endif  // INGAMEMENUARCHIVES_H
