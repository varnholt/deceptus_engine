#include "layer.h"


void Layer::draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const
{
   // TODO: use layer blendmode
   target.draw(*mSprite, {sf::BlendAlpha});
}


void Layer::show()
{
   mVisible = true;
}


void Layer::hide()
{
   mVisible = false;
}
