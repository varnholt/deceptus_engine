#ifndef IMAGELAYER_H
#define IMAGELAYER_H

#include <SFML/Graphics.hpp>


struct ImageLayer
{
  sf::Sprite mSprite;
  sf::Texture mTexture;
  sf::BlendMode mBlendMode = sf::BlendAdd;
  int32_t mZ = 0;
};

#endif // IMAGELAYER_H
