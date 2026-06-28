#include "gammashader.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <iostream>

void GammaShader::initialize()
{
   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/brightness.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading gamma shader";
      return;
   }
   _gamma_shader = std::move(*loaded);

   _uniform_gamma   = _gamma_shader->getUniformLocation("gamma");
   _uniform_texture = _gamma_shader->getUniformLocation("texture");
}

void GammaShader::update()
{
   if (!_gamma_shader.has_value() || !_uniform_gamma.has_value())
   {
      return;
   }
   const float gamma = 2.2f - (GameConfiguration::getInstance()._brightness - 0.5f);
   _gamma_shader->setUniform(*_uniform_gamma, gamma);
}

void GammaShader::setTexture(const sf::Texture& texture)
{
   if (!_gamma_shader.has_value() || !_uniform_texture.has_value())
   {
      return;
   }
   (void)_gamma_shader->setUniform(*_uniform_texture, texture);
}

const sf::Shader& GammaShader::getGammaShader() const
{
   return *_gamma_shader;
}
