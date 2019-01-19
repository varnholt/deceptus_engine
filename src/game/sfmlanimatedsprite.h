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

#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "game/constants.h"
#include "sfmlanimation.h"


class SpriteAnimation : public sf::Sprite
{

public:

   explicit SpriteAnimation(sf::Time mFrameTime = sf::seconds(0.2f), bool mPaused = false, bool mLooped = true);

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
   void update(float dt);

   void play();
   void play(const Animation& mAnimation);
   void pause();
   void stop();

   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;

   void setFrame(int32_t newFrame, bool resetTime = true);

   void incrementElapsed(int ms);

   AnimationType mType = AnimationType::Invalid;
   std::vector<sf::IntRect> mFrames;
   sf::Texture mTexture;
   sf::Time mFrameTime;
   sf::Time mCurrentTime;
   int32_t mCurrentFrame = 0;
   int32_t mPreviousFrame = -1;
   bool mPaused = false;
   bool mLooped = false;
   sf::Vertex mVertices[4];
   int mElapsed = 0;
};

