#pragma once

#include <SFML/Graphics.hpp>

#include <optional>

//! \brief Free functions that hide the VRSFML (WASM) vs. vanilla SFML3 (desktop) API divide
//!        behind a single call site.
//!
//! Every function here is a trivial template whose body is selected by #ifdef DECEPTUS_VRSFML
//! at compile time. Each platform's build only ever instantiates one branch, so there is no
//! runtime dispatch of any kind (no virtual calls, no std::function, no branch) — the compiled
//! output is identical to writing the platform-specific call inline. Call sites should read the
//! same on both platforms; only this header knows about the underlying API split.
namespace sfcompat
{

/// \brief sets the position of a transformable object (sprite, shape, text, ...).
template <typename Drawable>
inline void setPosition(Drawable& drawable, const sf::Vector2f& position)
{
#ifdef DECEPTUS_VRSFML
   drawable.position = position;
#else
   drawable.setPosition(position);
#endif
}

/// \brief returns the position of a transformable object.
template <typename Drawable>
inline sf::Vector2f getPosition(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.position;
#else
   return drawable.getPosition();
#endif
}

/// \brief returns the centre of a view.
inline sf::Vector2f getViewCenter(const sf::View& view)
{
#ifdef DECEPTUS_VRSFML
   return view.center;
#else
   return view.getCenter();
#endif
}

/// \brief returns the size of a view.
inline sf::Vector2f getViewSize(const sf::View& view)
{
#ifdef DECEPTUS_VRSFML
   return view.size;
#else
   return view.getSize();
#endif
}

/// \brief restricts a view's rendering to part of the target, as a fraction of it.
/// \note the scissor clips the rasteriser without touching the projection, so geometry lands on
///       exactly the same pixels as it would through the unrestricted view.
inline void setViewScissor(sf::View& view, const sf::FloatRect& scissor)
{
#ifdef DECEPTUS_VRSFML
   view.scissor = scissor;
#else
   view.setScissor(scissor);
#endif
}

/// \brief sets the origin of a transformable object.
template <typename Drawable>
inline void setOrigin(Drawable& drawable, const sf::Vector2f& origin)
{
#ifdef DECEPTUS_VRSFML
   drawable.origin = origin;
#else
   drawable.setOrigin(origin);
#endif
}

/// \brief returns the origin of a transformable object.
template <typename Drawable>
inline sf::Vector2f getOrigin(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.origin;
#else
   return drawable.getOrigin();
#endif
}

/// \brief sets the scale of a transformable object.
template <typename Drawable>
inline void setScale(Drawable& drawable, const sf::Vector2f& scale)
{
#ifdef DECEPTUS_VRSFML
   drawable.scale = scale;
#else
   drawable.setScale(scale);
#endif
}

/// \brief returns the scale of a transformable object.
template <typename Drawable>
inline sf::Vector2f getScale(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.scale;
#else
   return drawable.getScale();
#endif
}

/// \brief sets the rotation of a transformable object.
template <typename Drawable>
inline void setRotation(Drawable& drawable, const sf::Angle& angle)
{
#ifdef DECEPTUS_VRSFML
   drawable.rotation = angle;
#else
   drawable.setRotation(angle);
#endif
}

/// \brief returns the rotation of a transformable object.
template <typename Drawable>
inline sf::Angle getRotation(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.rotation;
#else
   return drawable.getRotation();
#endif
}

/// \brief sets the color of a sprite, shape, or text.
template <typename Drawable>
inline void setColor(Drawable& drawable, const sf::Color& color)
{
#ifdef DECEPTUS_VRSFML
   drawable.color = color;
#else
   drawable.setColor(color);
#endif
}

/// \brief returns the color of a sprite, shape, or text.
template <typename Drawable>
inline sf::Color getColor(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.color;
#else
   return drawable.getColor();
#endif
}

/// \brief sets the texture rect of a sprite.
template <typename Drawable, typename Rect>
inline void setTextureRect(Drawable& drawable, const Rect& rect)
{
#ifdef DECEPTUS_VRSFML
   drawable.textureRect = rect;
#else
   drawable.setTextureRect(rect);
#endif
}

/// \brief returns the texture rect of a sprite.
template <typename Drawable>
inline auto getTextureRect(const Drawable& drawable)
{
#ifdef DECEPTUS_VRSFML
   return drawable.textureRect;
#else
   return drawable.getTextureRect();
#endif
}

/// \brief returns the intersection of two rects, or std::nullopt if they do not intersect.
template <typename Rect>
inline std::optional<Rect> findIntersection(const Rect& a, const Rect& b)
{
#ifdef DECEPTUS_VRSFML
   const auto result = sf::findIntersection(a, b);
   return result.hasValue() ? std::optional<Rect>{*result} : std::nullopt;
#else
   return a.findIntersection(b);
#endif
}

/// \brief returns a zero-length sf::Time.
inline sf::Time timeZero()
{
#ifdef DECEPTUS_VRSFML
   return sf::Time{};
#else
   return sf::Time::Zero;
#endif
}

}  // namespace sfcompat
