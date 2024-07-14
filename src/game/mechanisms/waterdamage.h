#ifndef WATERDAMAGE_H
#define WATERDAMAGE_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <chrono>
#include <optional>

class WaterDamage : public GameNode, public GameMechanism
{
public:
   WaterDamage(GameNode* parent = nullptr);

   void update(const sf::Time& dt) override;
   void setup(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   void damage();

   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   std::optional<HighResTimePoint> _last_hurt_timepoint;
   HighResDuration _hurt_interval;
   int32_t _hurt_amount{0};
};

#endif  // WATERDAMAGE_H
