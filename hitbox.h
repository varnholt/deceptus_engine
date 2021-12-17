#pragma once

#include <SFML/Graphics.hpp>

class Hitbox
{

public:
   Hitbox(const sf::FloatRect& rect);
   const sf::FloatRect& getRect() const;
   sf::FloatRect getRectTranslated() const;

private:
   sf::FloatRect _rect_px;
   sf::Vector2f _offset_px;
};


