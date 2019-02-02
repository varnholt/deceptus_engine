#include "animation.h"

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
void Animation::play()
{
   mPaused = false;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::pause()
{
   mPaused = true;
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::seekToStart()
{
   mPreviousFrame = -1;
   mCurrentFrame = 0;
   setFrame(mCurrentFrame);
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::stop()
{
   mPaused = true;
   seekToStart();
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect Animation::getLocalBounds() const
{
   sf::IntRect rect = mFrames[static_cast<size_t>(mCurrentFrame)];
   return sf::FloatRect(
      0.f,
      0.f,
      static_cast<float>(std::abs(rect.width)),
      static_cast<float>(std::abs(rect.height))
   );
}


//----------------------------------------------------------------------------------------------------------------------
sf::FloatRect Animation::getGlobalBounds() const
{
   return getTransform().transformRect(getLocalBounds());
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::setFrame(int32_t, bool resetTime)
{
   if (mFrames.size() > 0)
   {
      sf::IntRect rect = mFrames[static_cast<size_t>(mCurrentFrame)];

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
void Animation::update(const sf::Time& dt)
{
   if (!mPaused)
   {
      mPreviousFrame = mCurrentFrame;
      mCurrentTime += dt;

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

      mElapsed += dt;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Animation::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
   states.transform *= getTransform();
   states.texture = mTexture.get();
   target.draw(mVertices, 4, sf::Quads, states);
}


