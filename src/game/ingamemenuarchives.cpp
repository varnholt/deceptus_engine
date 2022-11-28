#include "ingamemenuarchives.h"

InGameMenuArchives::InGameMenuArchives()
{
   _filename = "data/game/archives.psd";
   load();
}

void InGameMenuArchives::draw(sf::RenderTarget& /*window*/, sf::RenderStates)
{
}

void InGameMenuArchives::update(const sf::Time& /*dt*/)
{
}

void InGameMenuArchives::show()
{
}

void InGameMenuArchives::hide()
{
}
