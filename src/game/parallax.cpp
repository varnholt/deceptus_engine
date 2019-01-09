// base
#include "parallax.h"

// sfml
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

// gl
#include <gl/GLU.h>


Parallax::Parallax()
{
}



void Parallax::initialize()
{
   if (sf::Shader::isAvailable())
   {
       // shaders are not available...
      mParallaxShader;

      // load both shaders
      if (mParallaxShader.loadFromFile(
            "data/shaders/parallax_vert.glsl",
            "data/shaders/parallax_frag.glsl"
         )
      )
      {
         printf("loaded shader: %d\n", mParallaxShader.getNativeHandle());

         // load textures
         bool ok1 = mTextures[0].loadFromFile("data/bg_1.png");
         bool ok2 = mTextures[1].loadFromFile("data/bg_2.png");
         bool ok3 = mTextures[2].loadFromFile("data/bg_3.png");
         bool ok4 = mTextures[3].loadFromFile("data/bg_4.png");

         for (int i = 0; i < 4; i++)
            mTextures[i].setSmooth(true);

         mGrass.loadFromFile("data/bg_0.png");
         mMoon.loadFromFile("data/bg_5.png");

         printf("loaded textures: %d, %d, %d, %d", ok1, ok2, ok3, ok4);
         fflush(stdout);

         mScales[0].x = 0.25f;
         mScales[0].y = 0.60f;

         mScales[1].x = 0.32f;
         mScales[1].y = 1.0f;

         mScales[2].x = 0.55f;
         mScales[2].y = 1.0f;

         mScales[3].x = 1.0f;
         mScales[3].y = 1.0f;

         mStarEffect = new StarEffect();
      }
      else
      {
         printf("error loading shader\n");
         fflush(stdout);
      }
   }
   else
   {
      printf("shader's not available\n");
      fflush(stdout);
   }
}


void Parallax::bind()
{
   sf::Shader::bind(&mParallaxShader);
}



/*

   +------------------------------------------------------------------------+- 1
   |                                                                        |
   |uv1 +---------------------+-  uv4                                       |
   |    |                     | |                                           |
   |    |          P          | n                                           |
   |    |                     | |                                           |
   |uv2 +---------------------+-  uv3                                       |
   |    \_________ m _________/                                             |
   |                                                                        |
   +------------------------------------------------------------------------+- 0
   |                                                                        |
   0                                                                        1

   if (p.x - m/2 < 0)
      p.x = m/2;
   if (p.x + m/2 > 1)
      p.x = 1 - m/2;

   if (p.y - n/2 < 0)
      p.y = n/2;
   if (p.y + n/2 > 1)
      p.y = 1 - n/2;

   uv1: P.x - m/2, P.y - n/2
   uv2: P.x - m/2, P.y + n/2
   uv3: P.x + m/2, p.y + n/2
   uv4: P.x + m/2, p.y - n/2

*/


void Parallax::drawOverlay(sf::Texture* texture, int xOffset, int yOffset)
{
   glBindTexture(GL_TEXTURE_2D, texture->getNativeHandle());

   glBegin(GL_TRIANGLES);

   glTexCoord2f(0, 0);
   glVertex2f(0 + xOffset, 0 + yOffset);

   glTexCoord2f(0, 1);
   glVertex2f(0 + xOffset, texture->getSize().y + yOffset);

   glTexCoord2f(1, 0);
   glVertex2f(texture->getSize().x + xOffset, yOffset);

   glTexCoord2f(0, 1);
   glVertex2f(0 + xOffset, texture->getSize().y + yOffset);

   glTexCoord2f(1, 1);
   glVertex2f(texture->getSize().x + xOffset, texture->getSize().y + yOffset);

   glTexCoord2f(1, 0);
   glVertex2f(texture->getSize().x + xOffset, yOffset);

   glEnd();
}




void Parallax::render(sf::RenderTarget &target, int w, int h)
{
   // bind();

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   for (int i = 3; i >= 0; i--)
   {
      if (i == 2)
      {
         drawOverlay(&mMoon, 1200, 0);
      }

      sf::Vector2f pos = mPosition;

      glBindTexture(GL_TEXTURE_2D, mTextures[i].getNativeHandle());
      // mParallaxShader.setParameter("texture", mTextures[i]);

      float m = mScales[i].x;
      float n = mScales[i].y;

      if (pos.x - m/2.0f < 0.0f)
         pos.x = m/2.0f;
      if (pos.x + m/2.0f > 1.0f)
         pos.x = 1.0f - m/2.0f;

      if (pos.y - n/2.0f < 0.0f)
         pos.y = n/2.0f;
      if (pos.y + n/2.0f > 1.0f)
         pos.y = 1.0f - n/2.0f;

      sf::Vector2f uv1;
      sf::Vector2f uv2;
      sf::Vector2f uv3;
      sf::Vector2f uv4;

      uv1.x = pos.x - m/2.0f;
      uv1.y = pos.y - n/2.0f;

      uv2.x = pos.x - m/2.0f;
      uv2.y = pos.y + n/2.0f;

      uv3.x = pos.x + m/2.0f;
      uv3.y = pos.y + n/2.0f;

      uv4.x = pos.x + m/2.0f;
      uv4.y = pos.y - n/2.0f;

      glColor3f(1.0f, 1.0f, 1.0f);

      glBegin(GL_TRIANGLES);

      glTexCoord2f(uv1.x, uv1.y);
      glVertex2f(0, 0);

      glTexCoord2f(uv2.x, uv2.y);
      glVertex2f(0, h);

      glTexCoord2f(uv4.x, uv4.y);
      glVertex2f(w, 0);

      glTexCoord2f(uv2.x, uv2.y);
      glVertex2f(0, h);

      glTexCoord2f(uv3.x, uv3.y);
      glVertex2f(w, h);

      glTexCoord2f(uv4.x, uv4.y);
      glVertex2f(w, 0);

      glEnd();
   }

   // mStarEffect->draw(target);

   // unbind();
}


void Parallax::unbind()
{
   sf::Shader::bind(NULL);
}


sf::Vector2f Parallax::getPosition() const
{
   return mPosition;
}


void Parallax::setPosition(float x, float y)
{
   // printf("update pos: %f, %f\n", x, y);
   mPosition.x = x;
   mPosition.y = y;
}


void Parallax::drawGrass()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);
   drawOverlay(&mGrass, 0, 600);
}


