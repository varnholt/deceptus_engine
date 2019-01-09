#ifndef SFMLDEBUGDRAW_H
#define SFMLDEBUGDRAW_H

#include <Box2D/Box2D.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <cmath>
#include <memory>

#include "game/constants.h"


class DebugDraw : public b2Draw
{
   public:

      DebugDraw(const std::shared_ptr<sf::RenderTarget>& window);

      static sf::Color GLColorToSFML(const b2Color &color, sf::Uint8 alpha = 255);
      static sf::Vector2f B2VecToSFVec(const b2Vec2 &vector);

      void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
      void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
      void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
      void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
      void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
      void DrawTransform(const b2Transform& xf);

      static DebugDraw* getInstance();

private:

      std::shared_ptr<sf::RenderTarget> mWindow;

      static DebugDraw* sInstance;
};
#endif //SFMLDEBUGDRAW_H
