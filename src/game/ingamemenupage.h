#ifndef INGAMEMENUPAGE_H
#define INGAMEMENUPAGE_H

#include <chrono>
#include <SFML/Graphics.hpp>

class InGameMenuPage
{
public:

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

   InGameMenuPage() = default;

   virtual void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) = 0;
   virtual void update(const sf::Time& dt) = 0;

   virtual void show() = 0;
   virtual void hide() = 0;

protected:

   // animation
   HighResTimePoint _time_show;
   HighResTimePoint _time_hide;
   bool _show_requested = false;
   bool _hide_requested = false;

};

#endif // INGAMEMENUPAGE_H
