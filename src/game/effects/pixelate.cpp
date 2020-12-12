#include "pixelate.h"

#include "texturepool.h"


Pixelate::Pixelate() :
    Effect("pixelate")
{
}


bool Pixelate::onLoad()
{
   mTexture = TexturePool::getInstance().get("resources/background.jpg");

   mSprite.setTexture(*mTexture);

   // Load the shader
   if (!mShader.loadFromFile("resources/pixelate.frag", sf::Shader::Fragment))
      return false;

   mShader.setUniform("texture", sf::Shader::CurrentTexture);

   return true;
}


void Pixelate::onUpdate(const sf::Time&, float x, float y)
{
    mShader.setUniform("pixel_threshold", (x + y) / 30);
}


void Pixelate::onDraw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.shader = &mShader;
    target.draw(mSprite, states);
}
