#ifndef PARALLAX_H
#define PARALLAX_H

// sfml
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

// game
#include "stareffect.h"

class Parallax
{
   public:

      Parallax();

      void initialize();

      void render(sf::RenderTarget& target, int w, int h);

      sf::Vector2f getPosition() const;
      void setPosition(float x, float y);

      void drawGrass();

protected:

      void drawOverlay(sf::Texture *texture, int xOffset, int yOffset);

      void bind();
      void unbind();

      sf::Shader mParallaxShader;

      sf::Vector2f mPosition;

      sf::Texture mTextures[4];
      sf::Vector2f mScales[4];

      sf::Texture mGrass;
      sf::Texture mMoon;

      StarEffect* mStarEffect;
};

#endif // PARALLAX_H
