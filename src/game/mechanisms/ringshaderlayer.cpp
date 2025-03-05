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
}

void RingShaderLayer::setEnabled(bool enabled)
{
   ShaderLayer::setEnabled(enabled);
}
