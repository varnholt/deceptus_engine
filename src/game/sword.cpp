#include "sword.h"


Sword::Sword()
{
   _type = WeaponType::Sword;
}


void Sword::draw(sf::RenderTarget& /*target*/)
{
}


void Sword::update(const sf::Time& /*time*/)
{
}


void Sword::initialize()
{
}


void Sword::useNow(const std::shared_ptr<b2World>& world, const b2Vec2& pos, const b2Vec2& dir)
{

}
