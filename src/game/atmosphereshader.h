#pragma once

#include <SFML/Graphics.hpp>

class AtmosphereShader
{
   public:
      AtmosphereShader(
         uint32_t textureWidth,
         uint32_t textureHeight
      );

      ~AtmosphereShader();

      void initialize();
      void update();

      const std::shared_ptr<sf::RenderTexture>& getRenderTexture() const;
      const sf::Shader& getShader() const;


   private:

      std::shared_ptr<sf::RenderTexture> mRenderTexture;

      sf::Shader mShader;
      sf::Texture mDistortionMap;

};
