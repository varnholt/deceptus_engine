#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;
struct TmxObjectGroup;

class StaticLight : public GameMechanism, public GameNode
{
public:
   StaticLight(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& target, sf::RenderTarget& color) override;
   void update(const sf::Time& time) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void deserialize(const GameDeserializeData& data);

   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _sprite;
   sf::BlendMode _blend_mode = sf::BlendAdd;
   sf::Color _color = {255, 255, 255, 255};
   float _flicker_amount = 1.0f;
   float _flicker_intensity = 0.0f;
   float _flicker_speed = 0.0f;
   float _flicker_alpha_amount = 1.0f;
   float _time_offset = 0.0f;
   int32_t _instance_number = 0;
};
