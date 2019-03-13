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

   sf::FloatRect getLocalBounds() const;
   sf::FloatRect getGlobalBounds() const;

   void setFrame(int32_t newFrame, bool resetTime = true);

   std::string mName;

   std::vector<sf::IntRect> mFrames;
   std::shared_ptr<sf::Texture> mTexture;

   sf::Vertex mVertices[4];

   std::vector<sf::Time> mFrameTimes;
   sf::Time mCurrentTime;
   sf::Time mElapsed;

   int32_t mCurrentFrame = 0;
   int32_t mPreviousFrame = -1;

   bool mPaused = false;
   bool mLooped = false;
   void seekToStart();
};

