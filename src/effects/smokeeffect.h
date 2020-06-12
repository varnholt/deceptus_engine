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

   static sf::Texture mTexture;
   static const std::string sLayerName;

   struct SmokeParticle
   {
      sf::Sprite mSprite;
      float mRot = 0.0f;
      float mRotationSpeed = 1.0f;
      float mTimeOffset = 0.0f;
   };

   std::array<SmokeParticle, 50> mParticles;
   int32_t mZ = 0;
   sf::Time mLastUpdateTime;
};

