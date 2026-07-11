#pragma once
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/MemoryInputStream.hpp>
#include <SFML/System/Path.hpp>
#include <SFML/System/Rect2.hpp>
#include <SFML/System/RectUtils.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Utf.hpp>
#include <SFML/System/Vec2.hpp>
#include <SFML/System/Vec3.hpp>

// Backwards compatibility aliases: VRSFML renamed these types
namespace sf
{
template <typename T>
using Vector2 = Vec2<T>;
using Vector2f = Vec2f;
using Vector2i = Vec2i;
using Vector2u = Vec2u;
template <typename T>
using Vector3 = Vec3<T>;
using Vector3f = Vec3f;
using Vector3i = Vec3i;
template <typename T>
using Rect = Rect2<T>;
using FloatRect = Rect2f;

// IntRect is kept as a struct (not a plain alias) so that it implicitly
// converts to FloatRect when assigned to sf::Sprite::textureRect, which
// changed from IntRect to FloatRect in VRSFML.
struct IntRect : Rect2i
{
   using Rect2i::Rect2i;
   IntRect(Vec2i position, Vec2i size) : Rect2i{position, size}
   {
   }  //!< construct from position + size (SFML2 compat)
   IntRect(const Rect2i& r) : Rect2i(r)
   {
   }  //!< construct from base
   operator FloatRect() const
   {
      return toRect2f();
   }  //!< implicit widening for textureRect
};
}  // namespace sf
