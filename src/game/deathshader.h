#pragma once

#include <SFML/Graphics.hpp>

class DeathShader
{
   public:

      DeathShader() = default;

      void initialize();
      void update(const sf::Time& dt);

      const sf::Shader& getShader() const;

   private:

      sf::Shader mShader;

      std::shared_ptr<sf::RenderTexture> mRenderTexture;

      sf::Texture mFlowField1;
      sf::Texture mFlowField2;

      float mElapsed = 0.0f;
};
