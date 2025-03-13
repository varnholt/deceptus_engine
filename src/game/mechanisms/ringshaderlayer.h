#ifndef RINGSHADERLAYER_H
#define RINGSHADERLAYER_H

#include "game/mechanisms/shaderlayer.h"

class RingShaderLayer : public ShaderLayer
{
public:
   RingShaderLayer(GameNode* parent = nullptr);
   void update(const sf::Time& dt) override final;
   void setEnabled(bool enabled) override final;
};

#endif  // RINGSHADERLAYER_H
