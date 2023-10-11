#include "playerstencil.h"

#include <iostream>
#include "SFML/Graphics.hpp"
#include "SFML/OpenGL.hpp"

void PlayerStencil::setupForeground()
{
   // always pass and replace with 1
   glStencilFunc(GL_ALWAYS, 1, 0xFF);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
}

void PlayerStencil::setupPlayer()
{
   // pass only if stencil value is 1
   glStencilFunc(GL_EQUAL, 1, 0xFF);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void PlayerStencil::enable()
{
   glEnable(GL_STENCIL_TEST);
}

void PlayerStencil::disable()
{
   glDisable(GL_STENCIL_TEST);
}

void PlayerStencil::clearStencilBuffer()
{
   glClear(GL_STENCIL_BUFFER_BIT);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void PlayerStencil::dump(const std::shared_ptr<sf::RenderTexture>& texture)
{
   const auto w = texture->getSize().x;
   const auto h = texture->getSize().y;

   std::vector<GLuint> stencilBuffer(w * h);

   // Read the stencil data from the current rendering context
   glReadPixels(0, 0, w, h, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &stencilBuffer[0]);

   sf::Image image;
   image.create(w, h);
   for (auto y = 0u; y < h; ++y)
   {
      for (auto x = 0u; x < w; ++x)
      {
         auto index = y * w + x;
         auto stencil_value = stencilBuffer[index];
         sf::Color color(stencil_value & 0xFF, 0, 0);  // Create a color based on the stencil value
         image.setPixel(x, y, color);
      }
   }

   if (image.saveToFile("stencil_debug.png"))
   {
      std::cout << "Image saved successfully!" << std::endl;
   }
   else
   {
      std::cout << "Failed to save the image." << std::endl;
   }
}
