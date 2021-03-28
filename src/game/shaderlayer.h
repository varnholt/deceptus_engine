#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>

struct TmxObject;

struct ShaderLayer
{
   void draw(sf::RenderTarget& target);

   sf::Shader _shader;
   sf::Sprite _sprite;
   sf::Vector2f _position;
   sf::Vector2f _size;
   std::shared_ptr<sf::Texture> _texture;
   int32_t _z = 0;

   static std::shared_ptr<ShaderLayer> deserialize(TmxObject* element);
};

