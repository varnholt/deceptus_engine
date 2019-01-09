#ifndef STAREFFECT_H
#define STAREFFECT_H

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>


class StarEffect
{

public:

   StarEffect();


   void draw(sf::RenderTarget &target);

   sf::Texture mStarTexture;
   sf::Sprite mStar;
};

#endif // STAREFFECT_H
