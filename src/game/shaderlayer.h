#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>

struct TmxObject;

struct ShaderLayer
{
   void draw(sf::RenderTarget& target);

   sf::Shader mShader;
   sf::Sprite mSprite;
   std::shared_ptr<sf::Texture> mTexture;
   sf::RenderTexture mRenderTexture;

   int32_t mZ = 0;

   static std::shared_ptr<ShaderLayer> deserialize(TmxObject* element);
};

