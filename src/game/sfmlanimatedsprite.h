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

#ifndef SFMLANIMATEDSPRITE_INCLUDE
#define SFMLANIMATEDSPRITE_INCLUDE

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "sfmlanimation.h"

class SfmlAnimatedSprite : public sf::Sprite
{

private:

   const SfmlAnimation* mAnimation = nullptr;
   sf::Time mFrameTime;
   sf::Time mCurrentTime;
   size_t mCurrentFrame = 0;
   size_t mPreviousFrame = -1;
   bool mPaused = false;
   bool mLooped = false;
   const sf::Texture* mTexture = nullptr;
   sf::Vertex mVertices[4];
   int mElapsed = 0;


public:

   explicit SfmlAnimatedSprite(sf::Time mFrameTime = sf::seconds(0.2f), bool mPaused = false, bool mLooped = true);

   void update(float dt);
   void setAnimation(const SfmlAnimation& mAnimation);
   void setFrameTime(sf::Time time);
   void play();
   void play(const SfmlAnimation& mAnimation);
   void pause();
   void stop();
   void setLooped(bool mLooped);
   void setColor(const sf::Color& color);
   const SfmlAnimation* getAnimation() const;
   size_t getCurrentFrame();
   size_t getPreviousFrame();
   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;
   bool isLooped() const;
   bool isPlaying() const;
   sf::Time getFrameTime() const;
   void setFrame(size_t newFrame, bool resetTime = true);
   void incrementElapsed(int ms);
   int getElapsed();


private:

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

};

#endif // ANIMATEDSPRITE_INCLUDE
