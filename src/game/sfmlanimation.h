#pragma once

#include <vector>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>


class SfmlAnimation
{

public:

   SfmlAnimation();

   void addFrame(sf::IntRect rect);
   void setSpriteSheet(const sf::Texture& texture);
   const sf::Texture* getSpriteSheet() const;
   size_t getSize() const;
   const sf::IntRect& getFrame(size_t n) const;


private:

   std::vector<sf::IntRect> mFrames;
   const sf::Texture* mTexture = nullptr;


};
