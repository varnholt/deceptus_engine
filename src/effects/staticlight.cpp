#include "staticlight.h"

#include "game/fbm.h"

const std::string StaticLight::sLayerName = "static_lights";


StaticLight::StaticLight()
 : Effect("static light")
{
}


void StaticLight::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int z) const
{
   for (auto light : mLights)
   {
      if (z != light->mZ)
      {
         continue;
      }

      auto lumen =
         fbm::mix(
           light->mColor.a,
           light->mFlickerAmount * 255.0f,
           1.0f - light->mFlickerAlphaAmount
         );

      sf::Color color{
         light->mColor.r,
         light->mColor.g,
         light->mColor.b,
         static_cast<sf::Uint8>(lumen)
      };

      light->mSprite.setColor(color);
      target.draw(light->mSprite, light->mBlendMode);
   }
}


void StaticLight::onDraw(sf::RenderTarget &/*target*/, sf::RenderStates /*states*/) const
{
}


void StaticLight::onUpdate(const sf::Time& time, float /*x*/, float /*y*/)
{
   auto y = 0;
   for (auto light : mLights)
   {
      light->mFlickerAmount =
          light->mFlickerIntensity
        * fbm::fbm(
            fbm::vec2{
              time.asSeconds() * light->mFlickerSpeed,
              y / static_cast<float>(mLights.size())
          }
        );

      y++;
   }
}


bool StaticLight::onLoad()
{
   return true;
}
