#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <memory>

#include "game/level/level.h"

namespace DebugDraw
{

/// \brief converts a box2d debug color to an sfml color with an optional alpha override.
/// \param color source box2d color in normalized rgb range.
/// \param alpha alpha channel to write into the resulting sfml color.
/// \return converted sfml color.
sf::Color glColorToSfml(const b2Color& color, uint8_t alpha = 255);

/// \brief converts a box2d vector in meters to pixel coordinates.
/// \param vector vector in box2d world units.
/// \return vector in pixel space.
sf::Vector2f vecB2S(const b2Vec2& vector);

/// \brief converts a vector in pixels to box2d meters.
/// \param vector vector in pixel space.
/// \return vector in box2d world units.
b2Vec2 vecS2B(const sf::Vector2f& vector);

/// \brief draws an outlined circle in pixel space.
/// \param target render target.
/// \param center circle center in pixels.
/// \param radius circle radius in pixels.
/// \param color outline color in box2d rgb format.
void drawCircle(sf::RenderTarget& target, const sf::Vector2f& center, float radius, const b2Color& color);

/// \brief draws an outlined circle from box2d coordinates.
/// \param target render target.
/// \param center circle center in meters.
/// \param radius circle radius in meters.
/// \param color outline color in box2d rgb format.
void drawCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Color& color);

/// \brief draws a line between two box2d points.
/// \param target render target.
/// \param p1 start point in meters.
/// \param p2 end point in meters.
/// \param color line color in box2d rgb format.
void drawLine(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

/// \brief draws a line between two pixel-space points.
/// \param target render target.
/// \param p1 start point in pixels.
/// \param p2 end point in pixels.
/// \param color line color in box2d rgb format.
void drawLine(sf::RenderTarget& target, const sf::Vector2f& p1, const sf::Vector2f& p2, const b2Color& color);

/// \brief draws a line strip from box2d vertices with a shared offset.
/// \param target render target.
/// \param vertices pointer to box2d vertices.
/// \param offset world-space offset added to each vertex.
/// \param vertex_count number of vertices in the array.
/// \param color line color in box2d rgb format.
void drawLines(sf::RenderTarget& target, const b2Vec2* vertices, const b2Vec2& offset, int32_t vertex_count, const b2Color& color);

/// \brief draws a line strip from a list of box2d points.
/// \param target render target.
/// \param lines points to connect in order.
/// \param color line color in box2d rgb format.
void drawLines(sf::RenderTarget& target, const std::vector<b2Vec2>& lines, const b2Color& color);

/// \brief draws a closed line loop from box2d vertices and an offset.
/// \param target render target.
/// \param vertices pointer to box2d vertices.
/// \param offset world-space offset added to each vertex.
/// \param vertex_count number of vertices in the array.
/// \param color line color in box2d rgb format.
void drawLineLoop(sf::RenderTarget& target, const b2Vec2* vertices, const b2Vec2& offset, int32_t vertex_count, const b2Color& color);

/// \brief draws a small cross marker at a box2d position.
/// \param target render target.
/// \param p point position in meters.
/// \param color marker color in box2d rgb format.
void drawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color);

/// \brief draws a small cross marker at a pixel-space position.
/// \param target render target.
/// \param p point position in pixels.
/// \param color marker color in box2d rgb format.
void drawPoint(sf::RenderTarget& target, const sf::Vector2f& p, const b2Color& color);

/// \brief draws an outlined polygon from box2d vertices.
/// \param target render target.
/// \param vertices pointer to polygon vertices in meters.
/// \param vertex_count number of polygon vertices.
/// \param color outline color in box2d rgb format.
void drawPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);

/// \brief draws a float rectangle with configurable outline and fill colors.
/// \param target render target.
/// \param rect rectangle in pixels.
/// \param color rectangle outline color.
/// \param fill_color rectangle fill color.
void drawRect(
   sf::RenderTarget& target,
   const sf::FloatRect& rect,
   const sf::Color& color = sf::Color::Blue,
   const sf::Color& fill_color = sf::Color::Transparent
);

/// \brief draws an integer rectangle with configurable outline and fill colors.
/// \param target render target.
/// \param rect rectangle in pixels.
/// \param color rectangle outline color.
/// \param fill_color rectangle fill color.
void drawRect(
   sf::RenderTarget& target,
   const sf::IntRect& rect,
   const sf::Color& color = sf::Color::Blue,
   const sf::Color& fill_color = sf::Color::Transparent
);

/// \brief applies outline and fill colors to a shape and draws it.
/// \param target render target.
/// \param shape shape instance to draw.
/// \param color outline color.
/// \param fill_color fill color.
void drawShape(
   sf::RenderTarget& target,
   sf::Shape& shape,
   const sf::Color& color = sf::Color::Red,
   const sf::Color& fill_color = sf::Color::Transparent
);

/// \brief draws a filled box2d circle and its orientation axis.
/// \param target render target.
/// \param center circle center in meters.
/// \param radius circle radius in meters.
/// \param axis direction used to render the axis line.
/// \param color fill and outline color in box2d rgb format.
void drawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);

/// \brief draws a filled polygon with a translucent interior and visible outline.
/// \param target render target.
/// \param vertices pointer to polygon vertices in meters.
/// \param vertex_count number of polygon vertices.
/// \param color polygon color in box2d rgb format.
void drawSolidPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);

/// \brief draws the x and y axes of a box2d transform for orientation debugging.
/// \param target render target.
/// \param xf transform whose basis vectors are visualized.
void drawTransform(sf::RenderTarget& target, const b2Transform& xf);

/// \brief draws visible physics bodies, fixtures, joints, and velocity vectors for the current level.
/// \param target render target.
/// \param level level that owns the box2d world to inspect.
void debugBodies(sf::RenderTarget& target, Level* level);

/// \brief draws camera focus and panic guide lines from the active camera system.
/// \param target render target.
void debugCameraSystem(sf::RenderTarget& target);

/// \brief draws hitboxes for nodes currently inside the screen rectangle, highlighting recent hits.
/// \param target render target.
void debugHitboxes(sf::RenderTarget& target);

/// \brief computes the world-space rectangle currently covered by the target view.
/// \param target render target.
/// \return screen rectangle in pixel coordinates.
sf::FloatRect getScreenRect(sf::RenderTarget& target);

}  // namespace DebugDraw
