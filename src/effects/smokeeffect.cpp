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
      particle.mRot += dt * 10.0f * particle.mRotationSpeed;
      particle.mSprite.setRotation(particle.mRot);
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

   // if (tmxObject->mProperties != nullptr)
   // {
   //    auto it = tmxObject->mProperties->mMap.find("color");
   //    if (it != tmxObject->mProperties->mMap.end())
   //    {
   //       rgba = TmxTools::color(it->second->mValueStr);
   //    }
   // }

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
      const auto r = static_cast<float>(std::rand() % 360);

      x += tmxObject->mX + tmxObject->mWidth / 2;
      y += tmxObject->mY + tmxObject->mHeight / 2;

      const auto sx = (std::rand() % 50 + 50) * 0.008f; // scale from 0..0.4
      const auto sy = (std::rand() % 50 + 50) * 0.008f;

      particle.mSprite.setScale(sx, sy);

      particle.mSprite.setRotation(r);
      particle.mSprite.setPosition(x, y);

      particle.mSprite.setTexture(mTexture);
      particle.mSprite.setColor(mColor);
      
      const auto bounds = particle.mSprite.getGlobalBounds();
      particle.mSprite.setOrigin(bounds.width * sx, bounds.height * sy);
   }

   return smokeEffect;
}

