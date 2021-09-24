#include "gammashader.h"

#include "gameconfiguration.h"

#include <iostream>


//----------------------------------------------------------------------------------------------------------------------
void GammaShader::initialize()
{
   if (!_gamma_shader.loadFromFile("data/shaders/brightness.frag", sf::Shader::Fragment))
   {
      std::cout << "error loading gamma shader" << std::endl;
      return;
   }
}


//----------------------------------------------------------------------------------------------------------------------
void GammaShader::update()
{
   float gamma = 2.2f - (GameConfiguration::getInstance()._brightness - 0.5f);
   _gamma_shader.setUniform("gamma", gamma);
}


//----------------------------------------------------------------------------------------------------------------------
void GammaShader::setTexture(const sf::Texture& texture)
{
   _gamma_shader.setUniform("texture", texture);
}


//----------------------------------------------------------------------------------------------------------------------
const sf::Shader& GammaShader::getGammaShader() const
{
   return _gamma_shader;
}
