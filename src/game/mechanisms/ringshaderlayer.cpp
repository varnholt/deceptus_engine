#include "ringshaderlayer.h"

namespace
{
struct RingShaderLayerRegister
{
   RingShaderLayerRegister()
   {
      ShaderLayer::registerCustomization("ring", [](GameNode* parent) { return std::make_shared<RingShaderLayer>(parent); });
   }
};

static RingShaderLayerRegister reg;
}  // namespace

RingShaderLayer::RingShaderLayer(GameNode* parent) : ShaderLayer(parent)
{
}

void RingShaderLayer::update(const sf::Time& dt)
{
   ShaderLayer::update(dt);

   if (_flash_duration > 0.0f)
   {
      _flash_elapsed += dt.asSeconds();
      _flash_intensity = std::max(1.0f - _flash_elapsed / _flash_duration, 0.0f);
      if (_flash_elapsed >= _flash_duration)
      {
         _flash_duration  = 0.0f;
         _flash_intensity = 0.0f;
      }
   }
}

void RingShaderLayer::flash(float red, float green, float blue, float duration_s)
{
   _flash_color    = sf::Glsl::Vec3{red, green, blue};
   _flash_duration = duration_s;
   _flash_elapsed  = 0.0f;
}

void RingShaderLayer::setEnabled(bool enabled)
{
   if (!enabled)
   {
      _disable_time = std::chrono::high_resolution_clock::now();
   }

   ShaderLayer::setEnabled(enabled);
}
