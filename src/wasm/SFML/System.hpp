#pragma once
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/MemoryInputStream.hpp>
#include <SFML/System/Path.hpp>
#include <SFML/System/Rect2.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Utf.hpp>
#include <SFML/System/Vec2.hpp>
#include <SFML/System/Vec3.hpp>

// Backwards compatibility aliases: VRSFML renamed these types
namespace sf {
    template<typename T>
    using Vector2 = Vec2<T>;
    using Vector2f = Vec2f;
    using Vector2i = Vec2i;
    using Vector2u = Vec2u;
    template<typename T>
    using Rect = Rect2<T>;
    using FloatRect = Rect2f;
    using IntRect = Rect2i;
}
