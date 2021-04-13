#pragma once

#include <array>

#include "rope.h"
#include "effects/lightsystem.h"

class RopeWithLight : public Rope
{
   public:

      RopeWithLight(GameNode* parent);

      void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;

      void setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world) override;


   private:

      sf::Sprite _lamp_sprite;
      sf::IntRect _lamp_sprite_rect_1;
      sf::IntRect _lamp_sprite_rect_2;
      std::shared_ptr<LightSystem::LightInstance> _light;
      std::array<uint8_t, 4> _color = {255, 255, 255, 100};
};

