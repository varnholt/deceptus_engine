#ifndef INGAMEMENUARCHIVES_H
#define INGAMEMENUARCHIVES_H

#include "ingamemenupage.h"

#include <SFML/Graphics.hpp>

class InGameMenuArchives : public InGameMenuPage
{
public:
   InGameMenuArchives();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
   void update(const sf::Time& dt) override;

   void show() override;
   void hide() override;
};

#endif // INGAMEMENUARCHIVES_H