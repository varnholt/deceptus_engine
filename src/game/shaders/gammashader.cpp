#include "gammashader.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <iostream>

void GammaShader::initialize()
{
#ifndef __EMSCRIPTEN__
   _gamma_shader.emplace();
   if (!_gamma_shader->loadFromFile("data/shaders/brightness.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading gamma shader";
      return;
   }
#endif
}

void GammaShader::update()
{
#ifndef __EMSCRIPTEN__
   float gamma = 2.2f - (GameConfiguration::getInstance()._brightness - 0.5f);
   _gamma_shader->setUniform("gamma", gamma);
#endif
}

void GammaShader::setTexture(const sf::Texture& texture)
{
#ifndef __EMSCRIPTEN__
   _gamma_shader->setUniform("texture", texture);
#endif
}

const sf::Shader& GammaShader::getGammaShader() const
{
   return *_gamma_shader;
}
