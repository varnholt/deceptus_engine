#include "effect.h"



Effect::Effect(const std::string& name) :
    _name(name)
{
}


const std::string& Effect::getName() const
{
    return _name;
}


void Effect::load()
{
    _is_loaded = onLoad();
}


void Effect::update(const sf::Time& time, float x, float y)
{
    if (_is_loaded)
    {
        onUpdate(time, x, y);
    }
}


void Effect::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (_is_loaded)
    {
        onDraw(target, states);
    }
}
