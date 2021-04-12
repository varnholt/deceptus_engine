#pragma once

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
      std::shared_ptr<LightSystem::LightInstance> _light;
};

