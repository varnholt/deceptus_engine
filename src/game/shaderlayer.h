#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>

struct TmxObject;

struct ShaderLayer
{
   void draw(sf::RenderTarget& target);

   mutable sf::Shader mShader;
   sf::Sprite mSprite;
   sf::Vector2f mPosition;
   sf::Vector2f mSize;
   std::shared_ptr<sf::Texture> mTexture;

   int32_t mZ = 0;

   static std::shared_ptr<ShaderLayer> deserialize(TmxObject* element);
};

