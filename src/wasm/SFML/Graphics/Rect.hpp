#pragma once
// VRSFML moved Rect to SFML/System/Rect2.hpp and renamed the type to Rect2.
// Pull in the compat aliases from the System shim so that SFML2-style
// sf::Rect<T>, sf::FloatRect, and sf::IntRect remain usable.
#include <SFML/System.hpp>
