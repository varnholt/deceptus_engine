#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <cmath>
#include <memory>

#include "game/constants.h"
#include "game/level.h"


namespace DebugDraw
{

sf::Color glColorToSfml(const b2Color &color, sf::Uint8 alpha = 255);
sf::Vector2f b2VecToSfml(const b2Vec2 &vector);

void DrawPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);
void DrawSolidPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertex_count, const b2Color& color);
void DrawCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Color& color);
void DrawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
void DrawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color);
void DrawSegment(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
void DrawTransform(sf::RenderTarget& target, const b2Transform& xf);

void debugBodies(sf::RenderTarget& target, Level* level);
void debugCameraSystem(sf::RenderTarget& target);
void drawShape(sf::RenderTarget& target, sf::Shape& shape, const sf::Color& color = sf::Color::Red);
void drawRect(sf::RenderTarget& target, const sf::IntRect& rect, const sf::Color& color = sf::Color::Blue);
void drawRect(sf::RenderTarget& target, const sf::FloatRect& rect, const sf::Color& color = sf::Color::Blue);

};
