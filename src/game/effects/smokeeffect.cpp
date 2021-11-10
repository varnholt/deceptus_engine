#include "smokeeffect.h"

#include "framework/math/fbm.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtools.h"
#include "texturepool.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <math.h>


namespace
{
sf::BlendMode mBlendMode = sf::BlendAdd;
}


SmokeEffect::SmokeEffect()
 : Effect("smoke effect"),
   _texture(TexturePool::getInstance().get("data/effects/smoke.png"))
{
   _is_loaded = true;
   _texture->setSmooth(true);
}


void SmokeEffect::drawToZ(sf::RenderTarget &target, sf::RenderStates /*states*/, int z) const
{
    if (z != 20)
    {
        return;
    }

   for (auto& particle : _particles)
   {
      target.draw(particle._sprite, mBlendMode);
   }
}


void SmokeEffect::onDraw(sf::RenderTarget &/*target*/, sf::RenderStates /*states*/) const
{
}


void SmokeEffect::onUpdate(const sf::Time& time, float /*x*/, float /*y*/)
{
   const auto dt = time.asSeconds() - _last_update_time.asSeconds();
   _last_update_time = time;

   for (auto& particle : _particles)
   {
      particle._rot += dt * 10.0f * particle._rot_dir;
      particle._sprite.setRotation(particle._rot);

      // fake z rotation
      const auto x = 0.5f * (1.0f + sin(particle._time_offset + time.asSeconds())) * particle._offset.x;
      const auto y = 0.5f * (1.0f + cos(particle._time_offset + time.asSeconds())) * particle._offset.y;

      particle._sprite.setPosition(particle._center.x + x, particle._center.y + y);
   }
}


bool SmokeEffect::onLoad()
{
   return true;
}


//-----------------------------------------------------------------------------
std::shared_ptr<SmokeEffect> SmokeEffect::deserialize(TmxObject* tmxObject, TmxObjectGroup* /*objectGroup*/)
{
   auto smokeEffect = std::make_shared<SmokeEffect>();

   if (tmxObject->_properties)
   {
      auto z = tmxObject->_properties->_map.find("z");
      if (z != tmxObject->_properties->_map.end())
      {
         smokeEffect->_z = tmxObject->_properties->_map["z"]->_value_int.value();
      }
   }

   const auto rangeX = static_cast<int32_t>(tmxObject->_width_px);
   const auto rangeY = static_cast<int32_t>(tmxObject->_height_px);

   for (auto& particle : smokeEffect->_particles)
   {
      auto x = static_cast<float>(std::rand() % rangeX - rangeX / 2);
      auto y = static_cast<float>(std::rand() % rangeY - rangeY / 2);
      const auto rotation = static_cast<float>(std::rand() % 360);
      const auto timeOffset = static_cast<float>(std::rand() % 100) * 0.01f * 2.0f * static_cast<float>(M_PI);

      const auto centerX = tmxObject->_x_px + tmxObject->_width_px / 2;
      const auto centerY = tmxObject->_y_px + tmxObject->_height_px / 2;

      const auto sx = (std::rand() % 50 + 50) * 0.008f; // scale from 0..0.4
      const auto sy = (std::rand() % 50 + 50) * 0.008f;

      particle._sprite.setScale(sx, sy);
      particle._sprite.setRotation(rotation);
      particle._sprite.setPosition(x + centerX, y + centerY);

      particle._rot_dir = static_cast<float>((std::rand() % 200) - 100) * 0.01f;
      particle._center = sf::Vector2f{centerX, centerY};
      particle._offset = sf::Vector2f{x, y};
      particle._time_offset = timeOffset;

      particle._sprite.setTexture(*smokeEffect->_texture);
      particle._sprite.setColor(sf::Color(255, 255, 255, 25));

      const auto bounds = particle._sprite.getGlobalBounds();
      particle._sprite.setOrigin(bounds.width * sx, bounds.height * sy);
   }

   return smokeEffect;
}
