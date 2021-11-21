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
   float _time_offset = 0.0f;
   float _uv_width = 1.0f;
   float _uv_height = 1.0f;

   static std::shared_ptr<ShaderLayer> deserialize(TmxObject* element);
};

