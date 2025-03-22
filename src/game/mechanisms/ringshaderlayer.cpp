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

   // update uniforms depending on enabled state

   // slightly decrease the effect value
   // effect *= 0.2;
   // slightly decrease the alpha value of the color
   // fragColor = vec4(col, 1.0);

   const auto now = std::chrono::high_resolution_clock::now();

   const auto elapsed = now - _disable_time;
}

void RingShaderLayer::setEnabled(bool enabled)
{
   if (!enabled)
   {
      _disable_time = std::chrono::high_resolution_clock::now();
   }

   ShaderLayer::setEnabled(enabled);
}
