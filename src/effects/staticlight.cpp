#include "staticlight.h"

#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxobjectgroup.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxtools.h"

#include "game/math/fbm.h"

#include <array>
#include <filesystem>


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
   for (auto& light : mLights)
   {
      light->mFlickerAmount =
          light->mFlickerIntensity
        * fbm::fbm(
            fbm::vec2{
              y + time.asSeconds() * light->mFlickerSpeed,
              light->mTimeOffset + y / static_cast<float>(mLights.size())
          }
        );

      y++;
   }
}


bool StaticLight::onLoad()
{
   return true;
}


//-----------------------------------------------------------------------------
std::shared_ptr<StaticLight::LightInstance> StaticLight::deserialize(TmxObject* tmxObject, TmxObjectGroup* objectGroup)
{
  // std::cout << "static light: " << objectGroup->mName << " at layer: " << objectGroup->mZ << std::endl;

   auto light = std::make_shared<StaticLight::LightInstance>();
   std::array<uint8_t, 4> rgba = {255, 255, 255, 255};
   std::string texture = "data/light/smooth.png";
   auto flickerIntensity = 0.0f;
   auto flickerAlphaAmount = 1.0f;
   auto flickerSpeed = 0.0f;

   if (tmxObject->mProperties != nullptr)
   {
      auto it = tmxObject->mProperties->mMap.find("color");
      if (it != tmxObject->mProperties->mMap.end())
      {
         rgba = TmxTools::color(it->second->mValueStr);
      }

      it = tmxObject->mProperties->mMap.find("texture");
      if (it != tmxObject->mProperties->mMap.end())
      {
         texture = (std::filesystem::path("data/light/") / it->second->mValueStr).string();
      }

      it = tmxObject->mProperties->mMap.find("flicker_intensity");
      if (it != tmxObject->mProperties->mMap.end())
      {
         flickerIntensity = it->second->mValueFloat;
      }

      it = tmxObject->mProperties->mMap.find("flicker_alpha_amount");
      if (it != tmxObject->mProperties->mMap.end())
      {
         flickerAlphaAmount = it->second->mValueFloat;
      }

      it = tmxObject->mProperties->mMap.find("flicker_speed");
      if (it != tmxObject->mProperties->mMap.end())
      {
         flickerSpeed = it->second->mValueFloat;
      }
   }

   light->mColor.r = rgba[0];
   light->mColor.g = rgba[1];
   light->mColor.b = rgba[2];
   light->mColor.a = rgba[3];
   light->mFlickerIntensity = flickerIntensity;
   light->mFlickerAlphaAmount = flickerAlphaAmount;
   light->mFlickerSpeed = flickerSpeed;
   light->mSprite.setColor(light->mColor);
   light->mTexture.loadFromFile(texture);
   light->mSprite.setTexture(light->mTexture);
   light->mSprite.setPosition(tmxObject->mX, tmxObject->mY);
   light->mZ = objectGroup->mZ;

   auto scaleX = tmxObject->mWidth / light->mTexture.getSize().x;
   auto scaleY = tmxObject->mHeight / light->mTexture.getSize().y;
   light->mSprite.scale(scaleX, scaleY);

   // init each light with a different time offset
   // probably passing the position itself to FBM would be enough
   std::srand(static_cast<uint32_t>(tmxObject->mX * tmxObject->mY));
   light->mTimeOffset = (std::rand() % 100) * 0.01f;

   return light;
}

