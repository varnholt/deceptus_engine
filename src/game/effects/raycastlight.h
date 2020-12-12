#pragma once

#include "effect.h"

#include <memory>
#include <vector>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>


struct TmxObject;

class RaycastLight : public Effect
{

public:

   struct LightInstance
   {
      b2Vec2 mPosMeters;
      float mIntensity = 1.0f;
      sf::Color mColor = {255, 255, 255, 80};
      bool enabled = true;
      std::vector<b2Vec2> mIntersections;
      float maxDist = 40.0f;

      std::shared_ptr<sf::Texture> mTexture;
      sf::Sprite mSprite;

      int mWidth = 256;
      int mHeight = 256;
      int mOffsetX = 0;
      int mOffsetY = 0;

      void updateSpritePosition();
   };

   std::vector<std::shared_ptr<LightInstance>> mLights;
   sf::Shader mShader;

   RaycastLight();

   static std::shared_ptr<RaycastLight::LightInstance> deserialize(TmxObject* tmxObject);

private:

   bool onLoad() override;
   void onUpdate(const sf::Time& time, float x, float y) override;
   void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;
   void debug() const;

   void drawLines(sf::RenderTarget& target, std::shared_ptr<LightInstance> light) const;
   void drawQuads(sf::RenderTarget &target, std::shared_ptr<LightInstance> light) const;
};

