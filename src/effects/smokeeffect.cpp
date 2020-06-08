#include "smokeeffect.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxobjectgroup.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxtools.h"

#include "game/fbm.h"

#include <array>
#include <filesystem>


sf::Texture SmokeEffect::mTexture;

const std::string SmokeEffect::sLayerName = "smoke_effect";

namespace
{
   sf::BlendMode mBlendMode = sf::BlendAdd;
   sf::Color mColor = {255, 255, 255, 10};
}


SmokeEffect::SmokeEffect()
 : Effect("smoke effect")
{
}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int /*z*/) const
{
   for (auto particle : mParticles)
   {
      // todo
      // apply translation

      // todo
      // apply rotation

      target.draw(particle.mSprite, mBlendMode);
   }
}


void SmokeEffect::onDraw(sf::RenderTarget &/*target*/, sf::RenderStates /*states*/) const
{
}


void SmokeEffect::onUpdate(const sf::Time& /*time*/, float /*x*/, float /*y*/)
{
}


bool SmokeEffect::onLoad()
{
   return true;
}


//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(TmxObject* /*tmxObject*/, TmxObjectGroup* /*objectGroup*/)
{
   // std::cout << "static light: " << objectGroup->mName << " at layer: " << objectGroup->mZ << std::endl;

   auto smokeEffect = std::make_shared<SmokeEffect>();
   std::string texture = "data/light/smoke.png";

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

   for (auto& particle : smokeEffect->mParticles)
   {
      const auto x = static_cast<float>(std::rand() % 500 - 250);
      const auto y = static_cast<float>(std::rand() % 500 - 250);
      const auto r = static_cast<float>(std::rand() % 360);

      const auto sx = (std::rand() % 100 + 100) * 0.01f; // scale from 0..2
      const auto sy = (std::rand() % 100 + 100) * 0.01f;

      particle.mSprite.setPosition(x, y);
      particle.mSprite.setRotation(r);

      particle.mSprite.scale(sx, sy);

      particle.mSprite.setTexture(mTexture);
      particle.mSprite.setColor(mColor);
   }

   return smokeEffect;
}

