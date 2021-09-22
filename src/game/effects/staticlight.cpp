#include "staticlight.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "framework/math/fbm.h"
#include "texturepool.h"

#include <array>
#include <filesystem>


const std::string StaticLight::sLayerName = "static_lights";


StaticLight::StaticLight()
 : Effect("static light")
{
}


void StaticLight::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int z) const
{
   for (const auto& light : mLights)
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

   if (tmxObject->_properties != nullptr)
   {
      auto it = tmxObject->_properties->_map.find("color");
      if (it != tmxObject->_properties->_map.end())
      {
         rgba = TmxTools::color(it->second->_value_string.value());
      }

      it = tmxObject->_properties->_map.find("texture");
      if (it != tmxObject->_properties->_map.end())
      {
         texture = (std::filesystem::path("data/light/") / it->second->_value_string.value()).string();
      }

      it = tmxObject->_properties->_map.find("flicker_intensity");
      if (it != tmxObject->_properties->_map.end())
      {
         flickerIntensity = it->second->_value_float.value();
      }

      it = tmxObject->_properties->_map.find("flicker_alpha_amount");
      if (it != tmxObject->_properties->_map.end())
      {
         flickerAlphaAmount = it->second->_value_float.value();
      }

      it = tmxObject->_properties->_map.find("flicker_speed");
      if (it != tmxObject->_properties->_map.end())
      {
         flickerSpeed = it->second->_value_float.value();
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
   light->mTexture = TexturePool::getInstance().get(texture);
   light->mSprite.setTexture(*light->mTexture);
   light->mSprite.setPosition(tmxObject->_x_px, tmxObject->_y_px);
   light->mZ = objectGroup->_z_index;

   auto scaleX = tmxObject->_width_px / light->mTexture->getSize().x;
   auto scaleY = tmxObject->_height_px / light->mTexture->getSize().y;
   light->mSprite.scale(scaleX, scaleY);

   // init each light with a different time offset
   // probably passing the position itself to FBM would be enough
   std::srand(static_cast<uint32_t>(tmxObject->_x_px * tmxObject->_y_px));
   light->mTimeOffset = (std::rand() % 100) * 0.01f;

   return light;
}

