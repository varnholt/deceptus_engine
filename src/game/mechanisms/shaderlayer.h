#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <filesystem>

struct TmxObject;

struct ShaderLayer : public GameMechanism, public GameNode
{
   ShaderLayer(GameNode* parent = nullptr);
   std::string_view objectName() const override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void checkUniforms(const std::string& shaderPath);

   sf::Shader _shader;
   sf::Vector2f _position;
   sf::Vector2f _size;
   sf::FloatRect _rect;
   std::shared_ptr<sf::Texture> _texture;
   float _time_offset = 0.0f;
   float _uv_width = 1.0f;
   float _uv_height = 1.0f;
   sf::Time _elapsed;

   bool _has_u_resolution = false;
   bool _has_u_uv_height = false;

   static std::shared_ptr<ShaderLayer> deserialize(GameNode* parent, const GameDeserializeData& data);

   // customization factory
   using FactoryFunction = std::shared_ptr<ShaderLayer>(GameNode* parent);
   static void registerCustomization(const std::string& id, const std::function<FactoryFunction>&);
};
