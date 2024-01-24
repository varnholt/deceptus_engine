#include "infooverlay.h"

InfoOverlay::InfoOverlay(GameNode* parent) : GameNode(parent)
{
}

void InfoOverlay::update(const sf::Time& dt)
{
}

void InfoOverlay::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
}

void InfoOverlay::setup(const GameDeserializeData& data)
{
}

std::optional<sf::FloatRect> InfoOverlay::getBoundingBoxPx()
{
   return std::nullopt;
}
