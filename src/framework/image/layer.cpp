#include "layer.h"

void Layer::draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const
{
   // TODO: use layer blendmode
   target.draw(*_sprite, {sf::BlendAlpha});
}

void Layer::show()
{
   _visible = true;
}

void Layer::hide()
{
   _visible = false;
}
