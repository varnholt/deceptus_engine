#include "effect.h"



Effect::Effect(const std::string& name) :
    mName(name)
{
}


const std::string& Effect::getName() const
{
    return mName;
}


void Effect::load()
{
    mIsLoaded = onLoad();
}


void Effect::update(const sf::Time& time, float x, float y)
{
    if (mIsLoaded)
    {
        onUpdate(time, x, y);
    }
}


void Effect::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (mIsLoaded)
    {
        onDraw(target, states);
    }
}
