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

   static const std::string sLayerName;

   struct SmokeParticle
   {
      sf::Sprite mSprite;
      float mRot = 0.0f;
      float mTimeOffset = 0.0f;
   };

   std::array<SmokeParticle, 20> mParticles;

   static std::shared_ptr<SmokeEffect> deserialize(TmxObject* tmxObject, TmxObjectGroup* objectGroup);


public:

   SmokeEffect();

   static sf::Texture mTexture;

   int mZ = 0;
   sf::Time mLastUpdateTime;

   void drawToZ(sf::RenderTarget& target, sf::RenderStates states, int z) const;
   void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;
   void onUpdate(const sf::Time& time, float x, float y) override;
   bool onLoad() override;
};

