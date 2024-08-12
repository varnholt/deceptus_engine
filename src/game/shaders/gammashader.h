#pragma once

#include <SFML/Graphics.hpp>

class GammaShader
{
public:
   GammaShader() = default;

   void initialize();
   void update();
   void setTexture(const sf::Texture& texture);

   const sf::Shader& getGammaShader() const;

private:
   sf::Shader _gamma_shader;
};
