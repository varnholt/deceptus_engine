#ifndef INFOLAYER_H
#define INFOLAYER_H

#include "bitmapfont.h"

#include "image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>


class InfoLayer
{
public:

   InfoLayer();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void drawDebugInfo(sf::RenderTarget& window);

   void setLoading(bool loading);

private:

   BitmapFont mFont;

   bool mLoading = false;
   sf::Time mShowTime;

   std::vector<std::shared_ptr<Layer>> mLayerStack;
   std::map<std::string, std::shared_ptr<Layer>> mLayers;
};

#endif // INFOLAYER_H
