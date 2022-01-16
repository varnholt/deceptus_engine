#pragma once

#include <SFML/Graphics.hpp>

struct Hitbox
{

   Hitbox(const sf::FloatRect& rect);
   sf::FloatRect getRectTranslated() const;

   sf::FloatRect _rect_px;
   sf::Vector2f _offset_px;
};


