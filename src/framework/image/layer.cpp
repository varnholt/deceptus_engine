#include "layer.h"

void Layer::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   // TODO: use layer blendmode
   states.texture = _texture.get();
   states.blendMode = sf::BlendAlpha;
   target.draw(*_sprite, states);
}

void Layer::show()
{
   _visible = true;
}

void Layer::hide()
{
   _visible = false;
}
