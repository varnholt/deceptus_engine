#ifndef INGAMEMENUPAGE_H
#define INGAMEMENUPAGE_H

#include <SFML/Graphics.hpp>

class InGameMenuPage
{
public:
   InGameMenuPage() = default;

   virtual void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) = 0;
   virtual void update(const sf::Time& dt) = 0;

protected:
   bool _requires_updates = false;
};

#endif // INGAMEMENUPAGE_H
