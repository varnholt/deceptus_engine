#pragma once

#include <SFML/Graphics.hpp>

#include "game/constants.h"

#include <memory>
#include <vector>


class Animation : public sf::Sprite
{

public:

   Animation() = default;

   virtual void draw(sf::RenderTarget& target, sf::RenderStates states = {}) const;
   void update(const sf::Time& dt);

   void play();
   void pause();
   void stop();
   void seekToStart();
   void setFrame(int32_t newFrame, bool resetTime = true);

   void setAlpha(uint8_t alpha);

   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;

   std::string mName;

   std::vector<sf::IntRect> mFrames;
   std::shared_ptr<sf::Texture> mTexture;

   sf::Vertex mVertices[4];

   sf::Time mCurrentTime;
   sf::Time mElapsed;
   sf::Time mOverallTime;

   int32_t mCurrentFrame = 0;
   int32_t mPreviousFrame = -1;

   bool mPaused = false;
   bool mLooped = false;
   bool mResetToFirstFrame = true;

   void setFrameTimes(const std::vector<sf::Time>& frameTimes);

private:
   std::vector<sf::Time> mFrameTimes;
};

