#pragma once

#include <array>

#include "gamedeserializedata.h"
#include "rope.h"
#include "effects/lightsystem.h"

class RopeWithLight : public Rope
{
   public:

      RopeWithLight(GameNode* parent);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;
      void setup(const GameDeserializeData& data) override;

   private:

      sf::Sprite _lamp_sprite;
      std::array<sf::IntRect, 3> _lamp_sprite_rects;
      // sf::IntRect _lamp_sprite_rect_1;
      // sf::IntRect _lamp_sprite_rect_2;
      std::shared_ptr<LightSystem::LightInstance> _light;
};

