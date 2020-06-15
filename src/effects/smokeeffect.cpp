#include "smokeeffect.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxobjectgroup.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxtools.h"

#include "game/fbm.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <math.h>


sf::Texture SmokeEffect::mTexture;

const std::string SmokeEffect::sLayerName = "smoke_effect";

namespace
{
   sf::BlendMode mBlendMode = sf::BlendAdd;
   sf::Color mColor = sf::Color(255, 255, 255, 25);
}


SmokeEffect::SmokeEffect()
 : Effect("smoke effect")
{
   mIsLoaded = true;
}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int z) const
{
    if (z != 20)
    {
        return;
    }

   for (auto& particle : mParticles)
   {
      target.draw(particle.mSprite, mBlendMode);
   }
}


void SmokeEffect::onDraw(sf::RenderTarget &/*target*/, sf::RenderStates /*states*/) const
{
}


void SmokeEffect::onUpdate(const sf::Time& time, float /*x*/, float /*y*/)
{
   const auto dt = time.asSeconds() - mLastUpdateTime.asSeconds();
   mLastUpdateTime = time;

   for (auto& particle : mParticles)
   {
      particle.mRot += dt * 10.0f * particle.mRotDir;
      particle.mSprite.setRotation(particle.mRot);

      // fake z rotation
      const auto x = 0.5f * (1.0f + sin(particle.mTimeOffset + time.asSeconds())) * particle.mOffset.x;
      const auto y = 0.5f * (1.0f + cos(particle.mTimeOffset + time.asSeconds())) * particle.mOffset.y;

      particle.mSprite.setPosition(particle.mCenter.x + x, particle.mCenter.y + y);
   }
}


bool SmokeEffect::onLoad()
{
   return true;
}


//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(TmxObject* tmxObject, TmxObjectGroup* /*objectGroup*/)
{
   // std::cout << "static light: " << objectGroup->mName << " at layer: " << objectGroup->mZ << std::endl;

   auto smokeEffect = std::make_shared<SmokeEffect>();
   std::string texture = "data/effects/smoke.png";

   if (tmxObject->mProperties)
   {
      auto z = tmxObject->mProperties->mMap.find("z");
      if (z != tmxObject->mProperties->mMap.end())
      {
         smokeEffect->mZ = tmxObject->mProperties->mMap["z"]->mValueInt;
         std::cout << "smoke effect layer has z: " << smokeEffect->mZ << std::endl;
      }
   }

   if (mTexture.getSize().x == 0)
   {
      mTexture.loadFromFile(texture);
      mTexture.setSmooth(true);
   }

   const auto rangeX = static_cast<int32_t>(tmxObject->mWidth);
   const auto rangeY = static_cast<int32_t>(tmxObject->mHeight);

   for (auto& particle : smokeEffect->mParticles)
   {
      auto x = static_cast<float>(std::rand() % rangeX - rangeX / 2);
      auto y = static_cast<float>(std::rand() % rangeY - rangeY / 2);
      const auto rotation = static_cast<float>(std::rand() % 360);
      const auto timeOffset = static_cast<float>(std::rand() % 100) * 0.01f * 2.0f * static_cast<float>(M_PI);

      const auto centerX = tmxObject->mX + tmxObject->mWidth / 2;
      const auto centerY = tmxObject->mY + tmxObject->mHeight / 2;

      const auto sx = (std::rand() % 50 + 50) * 0.008f; // scale from 0..0.4
      const auto sy = (std::rand() % 50 + 50) * 0.008f;

      particle.mSprite.setScale(sx, sy);
      particle.mSprite.setRotation(rotation);
      particle.mSprite.setPosition(x + centerX, y + centerY);

      particle.mRotDir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;
      particle.mCenter = sf::Vector2f{centerX, centerY};
      particle.mOffset = sf::Vector2f{x, y};
      particle.mTimeOffset = timeOffset;

      particle.mSprite.setTexture(mTexture);
      particle.mSprite.setColor(mColor);

      const auto bounds = particle.mSprite.getGlobalBounds();
      particle.mSprite.setOrigin(bounds.width * sx, bounds.height * sy);
   }

   return smokeEffect;
}

