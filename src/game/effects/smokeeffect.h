#pragma once

#include "effect.h"

#include <SFML/Graphics.hpp>

#include <array>
#include <memory>
#include <vector>


struct TmxObject;
struct TmxObjectGroup;

class SmokeEffect : public Effect
{

public:

   SmokeEffect();
   void drawToZ(sf::RenderTarget& target, sf::RenderStates states, int z) const;
   void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;
   void onUpdate(const sf::Time& time, float x, float y) override;
   bool onLoad() override;

   static std::shared_ptr<SmokeEffect> deserialize(TmxObject* tmxObject, TmxObjectGroup* objectGroup);

private:

   std::shared_ptr<sf::Texture> _texture;

   struct SmokeParticle
   {
      sf::Sprite _sprite;
      float _rot = 0.0f;
      float _rot_dir = 1.0f;
      float _time_offset = 0.0f;

      sf::Vector2f _offset;
      sf::Vector2f _center;
   };

   std::array<SmokeParticle, 50> _particles;
   int32_t _z = 0;
   sf::Time _last_update_time;
};

