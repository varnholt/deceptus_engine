#include "sfmlanimation.h"


//----------------------------------------------------------------------------------------------------------------------
SfmlAnimation::SfmlAnimation()
{
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimation::addFrame(sf::IntRect rect)
{
   mFrames.push_back(rect);
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimation::setSpriteSheet(const sf::Texture& texture)
{
   this->mTexture = &texture;
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Texture* SfmlAnimation::getSpriteSheet() const
{
   return mTexture;
}


//----------------------------------------------------------------------------------------------------------------------
size_t SfmlAnimation::getSize() const
{
   return mFrames.size();
}


//----------------------------------------------------------------------------------------------------------------------
const sf::IntRect& SfmlAnimation::getFrame(size_t n) const
{
   return mFrames[n];
}
