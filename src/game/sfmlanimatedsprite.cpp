////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Maximilian Wagenbach (aka. Foaly) (foaly.f@web.de)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
// you must not claim that you wrote the original software.
// If you use this software in a product, an acknowledgment
// in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
// and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#include "sfmlanimatedsprite.h"

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
SfmlAnimatedSprite::SfmlAnimatedSprite(sf::Time frameTime, bool paused, bool looped)
{
   mFrameTime = frameTime;
   mPaused = paused;
   mLooped = looped;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::setAnimation(const SfmlAnimation &animation)
{
   this->mAnimation = &animation;
   mTexture = this->mAnimation->getSpriteSheet();

   mPreviousFrame = -1;
   mCurrentFrame = 0;
   setFrame(mCurrentFrame);
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::setFrameTime(sf::Time time)
{
   mFrameTime = time;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::play()
{
   mPaused = false;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::play(const SfmlAnimation &animation)
{
   if (getAnimation() != &animation)
   {
      setAnimation(animation);
   }

   play();
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::pause()
{
   mPaused = true;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::stop()
{
   mPaused = true;

   mPreviousFrame = -1;
   mCurrentFrame = 0;
   setFrame(mCurrentFrame);
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::setLooped(bool looped)
{
    this->mLooped = looped;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::setColor(const sf::Color& color)
{
   mVertices[0].color = color;
   mVertices[1].color = color;
   mVertices[2].color = color;
   mVertices[3].color = color;
}


//----------------------------------------------------------------------------------------------------------------------
const SfmlAnimation* SfmlAnimatedSprite::getAnimation() const
{
   return mAnimation;
}


//----------------------------------------------------------------------------------------------------------------------
size_t SfmlAnimatedSprite::getCurrentFrame()
{
   return mCurrentFrame;
}


//----------------------------------------------------------------------------------------------------------------------
size_t SfmlAnimatedSprite::getPreviousFrame()
{
   return mPreviousFrame;
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect SfmlAnimatedSprite::getLocalBounds() const
{
   sf::IntRect rect = mAnimation->getFrame(mCurrentFrame);

   float width  = static_cast<float>(std::abs(rect.width));
   float height = static_cast<float>(std::abs(rect.height));

   return sf::FloatRect(0.f, 0.f, width, height);
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect SfmlAnimatedSprite::getGlobalBounds() const
{
   return getTransform().transformRect(getLocalBounds());
}


//----------------------------------------------------------------------------------------------------------------------
bool SfmlAnimatedSprite::isLooped() const
{
   return mLooped;
}


//----------------------------------------------------------------------------------------------------------------------
bool SfmlAnimatedSprite::isPlaying() const
{
   return !mPaused;
}


//----------------------------------------------------------------------------------------------------------------------
sf::Time SfmlAnimatedSprite::getFrameTime() const
{
   return mFrameTime;
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::setFrame(size_t, bool resetTime)
{
   if (mAnimation && mAnimation->getSize() > 0)
   {
      sf::IntRect rect = mAnimation->getFrame(mCurrentFrame);

      mVertices[0].position = sf::Vector2f(0.f, 0.f);
      mVertices[1].position = sf::Vector2f(0.f, static_cast<float>(rect.height));
      mVertices[2].position = sf::Vector2f(static_cast<float>(rect.width), static_cast<float>(rect.height));
      mVertices[3].position = sf::Vector2f(static_cast<float>(rect.width), 0.f);

      float l =     static_cast<float>(rect.left) + 0.0001f;
      float r = l + static_cast<float>(rect.width);
      float t =     static_cast<float>(rect.top);
      float b = t + static_cast<float>(rect.height);

      mVertices[0].texCoords = sf::Vector2f(l, t);
      mVertices[1].texCoords = sf::Vector2f(l, b);
      mVertices[2].texCoords = sf::Vector2f(r, b);
      mVertices[3].texCoords = sf::Vector2f(r, t);
   }

   if (resetTime)
   {
      this->mCurrentTime = sf::Time::Zero;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::update(float dt)
{
   if (!mPaused && mAnimation)
   {
      mPreviousFrame = mCurrentFrame;

      mCurrentTime += sf::seconds(dt);

      // if current time is bigger then the frame time advance one frame
      if (mCurrentTime >= mFrameTime)
      {
         // reset time, but keep the remainder
         mCurrentTime = sf::microseconds(mCurrentTime.asMicroseconds() % mFrameTime.asMicroseconds());

         if (mCurrentFrame + 1 < mAnimation->getSize())
         {
            mCurrentFrame++;
         }
         else
         {
            mCurrentFrame = 0;

            if (!mLooped)
            {
               mPaused = true;
            }
         }

         setFrame(mCurrentFrame, false);
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   if (mAnimation && mTexture)
   {
      states.transform *= getTransform();
      states.texture = mTexture;
      target.draw(mVertices, 4, sf::Quads, states);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void SfmlAnimatedSprite::incrementElapsed(int ms)
{
   mElapsed += ms;
}


//----------------------------------------------------------------------------------------------------------------------
int SfmlAnimatedSprite::getElapsed()
{
   return mElapsed;
}

