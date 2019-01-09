// base
#include "stareffect.h"

// sfml
#include <SFML/Graphics/RenderTarget.hpp>

// game
#include "globalclock.h"


StarEffect::StarEffect()
{
   mStarTexture.loadFromFile("data/star_flare_default.png");
   mStar.setTexture(mStarTexture);
   mStar.setPosition(50, 1200);
   mStar.scale(0.33f, 0.33f);
}


void StarEffect::draw(sf::RenderTarget& target)
{
   float time = GlobalClock::getInstance()->getElapsedTimeInMs() * 0.001f;

   float color = 0.5f * (sin(time) + 1.0f);

   color *= 255.0f;
   mStar.setColor(sf::Color(color, color, color, color));// alpha * 255.0f));

   sf::RenderStates state;
   state.blendMode = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::One);

   target.draw(mStar, state);
}
