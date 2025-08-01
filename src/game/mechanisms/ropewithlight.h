#pragma once

#include <array>

#include "game/effects/lightsystem.h"
#include "game/io/gamedeserializedata.h"
#include "game/mechanisms/rope.h"

class RopeWithLight : public Rope
{
public:
   RopeWithLight(GameNode* parent);
   std::string_view objectName() const override;

   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void setup(const GameDeserializeData& data) override;

private:
   std::unique_ptr<sf::Sprite> _lamp_sprite;
   std::array<sf::IntRect, 3> _lamp_sprite_rects;
   // sf::IntRect _lamp_sprite_rect_1;
   // sf::IntRect _lamp_sprite_rect_2;
   std::shared_ptr<LightSystem::LightInstance> _light;
};
