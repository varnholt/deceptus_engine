#include "screentransitioneffect.h"


void ScreenTransitionEffect::start()
{
}


void ScreenTransitionEffect::done()
{
   _effect_ended();
}


void ScreenTransitionEffect::update(const sf::Time& /*dt*/)
{
}


void ScreenTransitionEffect::draw(const std::shared_ptr<sf::RenderTexture>& /*window*/)
{
}

