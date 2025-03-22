#ifndef RINGSHADERLAYER_H
#define RINGSHADERLAYER_H

#include "game/mechanisms/shaderlayer.h"

class RingShaderLayer : public ShaderLayer
{
public:
   RingShaderLayer(GameNode* parent = nullptr);
   void update(const sf::Time& dt) override final;
   void setEnabled(bool enabled) override final;

private:
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   HighResTimePoint _disable_time{};
};

#endif  // RINGSHADERLAYER_H
