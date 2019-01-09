#ifndef INFOLAYER_H
#define INFOLAYER_H

#include "bitmapfont.h"
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>


class InfoLayer
{
public:

   BitmapFont mFont;
   sf::Texture mHeartTexture;
   sf::Sprite mHeartSprite;


   InfoLayer();

   void draw(sf::RenderTarget& window);

   void drawDebugInfo(sf::RenderTarget& window);
};

#endif // INFOLAYER_H
