#include "playerstencil.h"

#include <format>
#include <iostream>
#include "SFML/Graphics.hpp"
#include "SFML/OpenGL.hpp"

#include "game/constants.h"

void PlayerStencil::replaceAllWithOne()
{
   glStencilFunc(GL_ALWAYS, 1, 0xFF);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
}

void PlayerStencil::keepIfOne()
{
   glStencilFunc(GL_EQUAL, 1, 0xFF);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void PlayerStencil::enableTest()
{
   glEnable(GL_STENCIL_TEST);
}

void PlayerStencil::disableTest()
{
   glDisable(GL_STENCIL_TEST);
}

void PlayerStencil::clearStencilBuffer()
{
   glClear(GL_STENCIL_BUFFER_BIT);
}

namespace
{
int32_t counter = 0;
int32_t frame_counter = 0;
}  // namespace

void PlayerStencil::dump(const std::shared_ptr<sf::RenderTexture>& texture)
{
   if ((++frame_counter % 60) != 0)
   {
      return;
   }

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
         const auto color = (stencil_value > 0) ? sf::Color::White : sf::Color::Black;
         image.setPixel(x, h - y - 1, color);
      }
   }

   const auto filename = std::format("stencil_debug_{}.png", counter++);
   image.saveToFile(filename);
}

bool PlayerStencil::isIgnored(int32_t /*z_index*/)
{
   return false;
}

int32_t PlayerStencil::getStartLayer()
{
   return static_cast<int32_t>(ZDepth::Player) + 1;
}

int32_t PlayerStencil::getStopLayer()
{
   return static_cast<int32_t>(ZDepth::ForegroundMax) - 1;
}
