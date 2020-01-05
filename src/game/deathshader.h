#pragma once

#include <SFML/Graphics.hpp>

class DeathShader
{
   public:

      DeathShader();

      void initialize();
      void update();

   private:

      sf::Shader mShader;
      std::shared_ptr<sf::RenderTexture> mRenderTexture;
      sf::Texture mFlowField1;
      sf::Texture mFlowField2;
};
