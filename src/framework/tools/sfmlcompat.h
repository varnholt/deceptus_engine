#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
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

/// \brief returns a view's scissor rectangle, as a fraction of the target.
/// \note an unscissored view reports the whole target, i.e. position 0,0 and size 1,1.
inline sf::FloatRect getViewScissor(const sf::View& view)
{
#ifdef DECEPTUS_VRSFML
   return view.scissor;
#else
   return view.getScissor();
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

/// \brief creates a render texture of the given size.
///
/// the two apis report a refusal differently: vrsfml returns an empty optional, vanilla sfml throws.
/// both arrive here as a null pointer, so a caller only has to check for one thing. what the
/// exception had to say is not passed on, since the vrsfml side has nothing to say in its place.
///
/// \param size size of the render texture in pixels.
/// \return the render texture, or nullptr when the graphics device refused it.
inline std::unique_ptr<sf::RenderTexture> createRenderTexture(const sf::Vector2u& size)
{
#ifdef DECEPTUS_VRSFML
   auto created = sf::RenderTexture::create(size);
   if (!created.hasValue())
   {
      return nullptr;
   }
   return std::make_unique<sf::RenderTexture>(std::move(*created));
#else
   try
   {
      return std::make_unique<sf::RenderTexture>(size);
   }
   catch (const std::exception&)
   {
      return nullptr;
   }
#endif
}

/// \brief creates an empty texture of the given size.
/// \param size size of the texture in pixels.
/// \return the texture, or nullptr when the graphics device refused it.
inline std::shared_ptr<sf::Texture> createTexture(const sf::Vector2u& size)
{
#ifdef DECEPTUS_VRSFML
   auto created = sf::Texture::create(size);
   if (!created.hasValue())
   {
      return nullptr;
   }
   return std::make_shared<sf::Texture>(std::move(*created));
#else
   try
   {
      return std::make_shared<sf::Texture>(size);
   }
   catch (const std::exception&)
   {
      return nullptr;
   }
#endif
}

/// \brief creates a text in the given font.
/// \param font font the text renders with; it has to outlive the text.
/// \return the text, with no string set on it yet.
inline sf::Text createText(const sf::Font& font)
{
#ifdef DECEPTUS_VRSFML
   return sf::Text(font, sf::Text::Data{});
#else
   return sf::Text(font);
#endif
}

/// \brief creates a sprite showing the whole of the given texture.
///
/// vanilla sfml takes the texture in the constructor and sizes the sprite from it. vrsfml keeps the
/// texture in the render states instead, so the sprite has to be told which part of it to show.
///
/// \param texture texture the sprite shows; it has to outlive the sprite.
/// \return the sprite, positioned at the origin.
inline std::shared_ptr<sf::Sprite> createSprite(const sf::Texture& texture)
{
#ifdef DECEPTUS_VRSFML
   auto sprite = std::make_shared<sf::Sprite>();
   const auto size = texture.getSize();
   sprite->textureRect = sf::FloatRect{{0.0f, 0.0f}, {static_cast<float>(size.x), static_cast<float>(size.y)}};
   return sprite;
#else
   return std::make_shared<sf::Sprite>(texture);
#endif
}

/// \brief draws one rectangle of a texture onto a target, alpha blended.
///
/// the two apis differ in where the texture comes from: vanilla sfml binds it to the sprite, vrsfml
/// carries it in the render states. the sprite is a local either way, so this is the whole operation
/// rather than a factory.
///
/// \param target target to draw onto.
/// \param texture texture to take the rectangle from.
/// \param source rectangle of the texture to draw.
/// \param position position on the target to draw it at.
/// \param scale scale to draw it at; the default draws it at its own size.
inline void drawTextureRegion(
   sf::RenderTarget& target,
   const sf::Texture& texture,
   const sf::IntRect& source,
   const sf::Vector2f& position,
   const sf::Vector2f& scale = {1.0f, 1.0f}
)
{
#ifdef DECEPTUS_VRSFML
   sf::Sprite sprite;
   sprite.textureRect = sf::FloatRect{
      {static_cast<float>(source.position.x), static_cast<float>(source.position.y)},
      {static_cast<float>(source.size.x), static_cast<float>(source.size.y)}
   };
   sprite.position = position;
   sprite.scale = scale;

   sf::RenderStates states;
   states.texture = &texture;
   states.blendMode = sf::BlendAlpha;
   target.draw(sprite, states);
#else
   sf::Sprite sprite(texture);
   sprite.setTextureRect(source);
   sprite.setPosition(position);
   sprite.setScale(scale);
   target.draw(sprite, sf::RenderStates{sf::BlendAlpha});
#endif
}

}  // namespace sfcompat
