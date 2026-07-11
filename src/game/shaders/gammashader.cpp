#include "gammashader.h"

#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"

#include <iostream>

void GammaShader::initialize()
{
#ifdef __EMSCRIPTEN__
   auto loaded = sf::Shader::loadFromFile({.fragmentPath = "data/shaders/brightness.frag"});
   if (!loaded.hasValue())
   {
      Log::Error() << "error loading gamma shader";
      return;
   }
   _gamma_shader = std::move(*loaded);

   auto get_ul = [&](const char* name) -> std::optional<sf::Shader::UniformLocation>
   {
      const auto result = _gamma_shader->getUniformLocation(name);
      return result.hasValue() ? std::optional{*result} : std::nullopt;
   };
   _uniform_gamma = get_ul("gamma");
   _uniform_texture = get_ul("u_texture");
#else
   if (!_gamma_shader.loadFromFile("data/shaders/brightness.frag", sf::Shader::Type::Fragment))
   {
      Log::Error() << "error loading gamma shader";
      return;
   }
#endif
}

void GammaShader::update()
{
#ifdef __EMSCRIPTEN__
   if (!_gamma_shader.has_value() || !_uniform_gamma.has_value())
   {
      return;
   }
   const float gamma = 2.2f - (GameConfiguration::getInstance()._brightness - 0.5f);
   _gamma_shader->setUniform(*_uniform_gamma, gamma);
#else
   float gamma = 2.2f - (GameConfiguration::getInstance()._brightness - 0.5f);
   _gamma_shader.setUniform("gamma", gamma);
#endif
}

void GammaShader::setTexture(const sf::Texture& texture)
{
#ifdef __EMSCRIPTEN__
   if (!_gamma_shader.has_value() || !_uniform_texture.has_value())
   {
      return;
   }
   (void)_gamma_shader->setUniform(*_uniform_texture, texture);
#else
   _gamma_shader.setUniform("texture", texture);
#endif
}

const sf::Shader& GammaShader::getGammaShader() const
{
#ifdef __EMSCRIPTEN__
   return *_gamma_shader;
#else
   return _gamma_shader;
#endif
}
