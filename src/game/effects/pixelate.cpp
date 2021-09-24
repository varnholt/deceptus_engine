#include "pixelate.h"

#include "texturepool.h"


Pixelate::Pixelate() :
    Effect("pixelate")
{
}


bool Pixelate::onLoad()
{
   _texture = TexturePool::getInstance().get("resources/background.jpg");

   _sprite.setTexture(*_texture);

   // Load the shader
   if (!_shader.loadFromFile("resources/pixelate.frag", sf::Shader::Fragment))
      return false;

   _shader.setUniform("texture", sf::Shader::CurrentTexture);

   return true;
}


void Pixelate::onUpdate(const sf::Time&, float x, float y)
{
    _shader.setUniform("pixel_threshold", (x + y) / 30);
}


void Pixelate::onDraw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.shader = &_shader;
    target.draw(_sprite, states);
}
