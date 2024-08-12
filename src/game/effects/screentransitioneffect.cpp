#include "screentransitioneffect.h"

#include <iostream>

void ScreenTransitionEffect::start()
{
}

void ScreenTransitionEffect::done()
{
   _done = true;
   _effect_ended();
}

void ScreenTransitionEffect::update(const sf::Time& /*dt*/)
{
}

void ScreenTransitionEffect::draw(const std::shared_ptr<sf::RenderTexture>& /*window*/)
{
}

// just here for debugging purposes
ScreenTransitionEffect::~ScreenTransitionEffect()
{
   // std::cout << "destroyed" << std::endl;
}
