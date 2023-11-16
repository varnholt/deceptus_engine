#pragma once

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <memory>

#include "game/level.h"

namespace DebugDraw
{

sf::Color glColorToSfml(const b2Color& color, sf::Uint8 alpha = 255);

sf::Vector2f vecB2S(const b2Vec2& vector);
b2Vec2 vecS2B(const sf::Vector2f& vector);

void drawCircle(sf::RenderTarget& target, const sf::Vector2f& center, float radius, const b2Color& color);
void drawCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Color& color);

void drawLine(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
void drawLine(sf::RenderTarget& target, const sf::Vector2f& p1, const sf::Vector2f& p2, const b2Color& color);

void drawLines(sf::RenderTarget& target, const b2Vec2* vertices, const b2Vec2& offset, int32_t vertex_count, const b2Color& color);
void drawLines(sf::RenderTarget& target, const std::vector<b2Vec2>& lines, const b2Color& color);

void drawLineLoop(sf::RenderTarget& target, const b2Vec2* vertices, const b2Vec2& offset, int32_t vertex_count, const b2Color& color);

void drawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color);
void drawPoint(sf::RenderTarget& target, const sf::Vector2f& p, const b2Color& color);

void drawPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);

void drawRect(
   sf::RenderTarget& target,
   const sf::FloatRect& rect,
   const sf::Color& color = sf::Color::Blue,
   const sf::Color& fill_color = sf::Color::Transparent
);

void drawRect(
   sf::RenderTarget& target,
   const sf::IntRect& rect,
   const sf::Color& color = sf::Color::Blue,
   const sf::Color& fill_color = sf::Color::Transparent
);

void drawShape(
   sf::RenderTarget& target,
   sf::Shape& shape,
   const sf::Color& color = sf::Color::Red,
   const sf::Color& fill_color = sf::Color::Transparent
);

void drawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);
void drawSolidPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);
void drawTransform(sf::RenderTarget& target, const b2Transform& xf);
void debugBodies(sf::RenderTarget& target, Level* level);
void debugCameraSystem(sf::RenderTarget& target);
void debugHitboxes(sf::RenderTarget& target);

sf::FloatRect getScreenRect(sf::RenderTarget& target);

}  // namespace DebugDraw
