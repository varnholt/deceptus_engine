#ifndef INGAMEMENUARCHIVES_H
#define INGAMEMENUARCHIVES_H

#include "ingamemenupage.h"
#include "layerdata.h"

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
};

#endif // INGAMEMENUARCHIVES_H
