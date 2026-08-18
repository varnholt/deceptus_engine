#pragma once
// VRSFML renamed Vector2→Vec2; provide SFML2 compat names
#include <SFML/System/Vec2.hpp>
namespace sf {
    template<typename T>
    using Vector2 = Vec2<T>;
    using Vector2f = Vec2f;
    using Vector2i = Vec2i;
    using Vector2u = Vec2u;
}
