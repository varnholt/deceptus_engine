#pragma once

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include <SFML/Graphics.hpp>
#include <filesystem>

struct TmxObject;

struct ShaderLayer : public GameMechanism, public GameNode
{
   ShaderLayer(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   sf::Shader _shader;
   sf::Sprite _sprite;
   sf::Vector2f _position;
   sf::Vector2f _size;
   std::shared_ptr<sf::Texture> _texture;
   float _time_offset = 0.0f;
   float _uv_width = 1.0f;
   float _uv_height = 1.0f;
   sf::Time _elapsed;

   static std::shared_ptr<ShaderLayer> deserialize(GameNode* parent, const GameDeserializeData& data);
};

