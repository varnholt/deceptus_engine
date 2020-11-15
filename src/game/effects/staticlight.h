#pragma once

#include "effect.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

struct TmxObject;
struct TmxObjectGroup;

class StaticLight : public Effect
{

public:

   static const std::string sLayerName;

   struct LightInstance
   {
      sf::Texture mTexture;
      sf::Sprite mSprite;
      sf::BlendMode mBlendMode = sf::BlendAdd;
      sf::Color mColor = {255, 255, 255, 255};
      int mZ = 0;
      float mFlickerAmount = 1.0f;
      float mFlickerIntensity = 0.0f;
      float mFlickerSpeed = 0.0f;
      float mFlickerAlphaAmount = 1.0f;
      float mTimeOffset = 0.0f;
   };

   std::vector<std::shared_ptr<LightInstance>> mLights;

   static std::shared_ptr<StaticLight::LightInstance> deserialize(TmxObject* tmxObject, TmxObjectGroup* objectGroup);


public:

   StaticLight();

   void drawToZ(sf::RenderTarget& target, sf::RenderStates states, int z) const;
   void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;
   void onUpdate(const sf::Time& time, float x, float y) override;
   bool onLoad() override;
};

