#pragma once

#include <SFML/Graphics.hpp>

class GammaShader
{
   public:
      GammaShader() = default;

      void initialize(const sf::Texture& texture);
      void update();

      const sf::Shader& getGammaShader() const;

   private:
      sf::Shader mGammaShader;
};
