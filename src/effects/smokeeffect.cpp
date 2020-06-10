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
   sf::BlendMode mBlendMode = sf::BlendMultiply;
   //sf::Color mColor = {255, 255, 255, 255};
}


SmokeEffect::SmokeEffect()
 : Effect("smoke effect")
{
   mIsLoaded = true;
}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int /*z*/) const
{
   //   sf::BlendMode masksCombining = sf::BlendAdd;
   //   masksCombining.colorEquation = sf::BlendMode::Add;
   //   masksCombining.alphaEquation = sf::BlendMode::ReverseSubtract;

   return;

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
      particle.mRot += dt;
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
   }

   constexpr auto range = 50;

   for (auto& particle : smokeEffect->mParticles)
   {
      auto x = static_cast<float>(std::rand() % range - range/2);
      auto y = static_cast<float>(std::rand() % range - range/2);
      const auto r = static_cast<float>(std::rand() % 360);

      x += tmxObject->mX;
      y += tmxObject->mY;

      const auto sx = (std::rand() % 100 + 100) * 0.002f; // scale from 0..0.4
      const auto sy = (std::rand() % 100 + 100) * 0.002f;

      particle.mSprite.setPosition(x, y);
      particle.mSprite.setRotation(r);

      particle.mSprite.scale(sx, sy);

      particle.mSprite.setTexture(mTexture);
      //particle.mSprite.setColor(mColor);
   }

   return smokeEffect;
}

