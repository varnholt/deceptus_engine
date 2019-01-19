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
SpriteAnimation::SpriteAnimation(sf::Time frameTime, bool paused, bool looped)
{
   mFrameTime = frameTime;
   mPaused = paused;
   mLooped = looped;
}


//----------------------------------------------------------------------------------------------------------------------
void SpriteAnimation::play()
{
   mPaused = false;
}


//----------------------------------------------------------------------------------------------------------------------
void SpriteAnimation::pause()
{
   mPaused = true;
}


//----------------------------------------------------------------------------------------------------------------------
void SpriteAnimation::stop()
{
   mPaused = true;

   mPreviousFrame = -1;
   mCurrentFrame = 0;
   setFrame(mCurrentFrame);
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect SpriteAnimation::getLocalBounds() const
{
   sf::IntRect rect = mFrames[mCurrentFrame];
   return sf::FloatRect(
      0.f,
      0.f,
      static_cast<float>(std::abs(rect.width)),
      static_cast<float>(std::abs(rect.height))
   );
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect SpriteAnimation::getGlobalBounds() const
{
   return getTransform().transformRect(getLocalBounds());
}


//----------------------------------------------------------------------------------------------------------------------
void SpriteAnimation::setFrame(int32_t, bool resetTime)
{
   if (mFrames.size() > 0)
   {
      sf::IntRect rect = mFrames[mCurrentFrame];

      const auto l =     static_cast<float>(rect.left) + 0.0001f;
      const auto r = l + static_cast<float>(rect.width);
      const auto t =     static_cast<float>(rect.top);
      const auto b = t + static_cast<float>(rect.height);

      mVertices[0].position = sf::Vector2f(0.f, 0.f);
      mVertices[1].position = sf::Vector2f(0.f, static_cast<float>(rect.height));
      mVertices[2].position = sf::Vector2f(static_cast<float>(rect.width), static_cast<float>(rect.height));
      mVertices[3].position = sf::Vector2f(static_cast<float>(rect.width), 0.f);

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
void SpriteAnimation::update(float dt)
{
   if (!mPaused)
   {
      mPreviousFrame = mCurrentFrame;

      mCurrentTime += sf::seconds(dt);

      // if current time is bigger then the frame time advance one frame
      if (mCurrentTime >= mFrameTime)
      {
         // reset time, but keep the remainder
         mCurrentTime = sf::microseconds(mCurrentTime.asMicroseconds() % mFrameTime.asMicroseconds());

         if (mCurrentFrame + 1 < static_cast<int32_t>(mFrames.size()))
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
void SpriteAnimation::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   states.transform *= getTransform();
   states.texture = &mTexture;
   target.draw(mVertices, 4, sf::Quads, states);
}


//----------------------------------------------------------------------------------------------------------------------
void SpriteAnimation::incrementElapsed(int ms)
{
   mElapsed += ms;
}
