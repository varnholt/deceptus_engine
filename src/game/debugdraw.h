#pragma once

#include <Box2D/Box2D.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <cmath>
#include <memory>

#include "game/constants.h"
#include "game/level.h"


class DebugDraw
{
   public:

      static sf::Color GLColorToSFML(const b2Color &color, sf::Uint8 alpha = 255);
      static sf::Vector2f B2VecToSFVec(const b2Vec2 &vector);

      static void DrawPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
      static void DrawSolidPolygon(sf::RenderTarget& target, const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
      static void DrawCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Color& color);
      static void DrawSolidCircle(sf::RenderTarget& target, const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
      static void DrawPoint(sf::RenderTarget& target, const b2Vec2& p, const b2Color& color);
      static void DrawSegment(sf::RenderTarget& target, const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
      static void DrawTransform(sf::RenderTarget& target, const b2Transform& xf);

      static void debugBodies(sf::RenderTarget& target, Level* level);
      static void debugCameraSystem(sf::RenderTarget& target);
      static void drawShape(sf::RenderTarget& target, sf::Shape& shape, const sf::Color& color = sf::Color::Red);
      static void drawRect(sf::RenderTarget& target, const sf::IntRect& rect, const sf::Color& color = sf::Color::Blue);
      static void drawRect(sf::RenderTarget& target, const sf::FloatRect& rect, const sf::Color& color = sf::Color::Blue);
};
