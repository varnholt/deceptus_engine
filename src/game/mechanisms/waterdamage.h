#ifndef WATERDAMAGE_H
#define WATERDAMAGE_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <chrono>
#include <optional>

/// \brief applies periodic damage while the player is submerged in water.
class WaterDamage : public GameNode, public GameMechanism
{
public:
   /// \brief creates a water-damage controller.
   /// \param parent owning game node in the scene graph.
   WaterDamage(GameNode* parent = nullptr);
   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "WaterDamage".
   std::string_view objectName() const override;

   /// \brief tracks submersion time and damages the player at configured intervals.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief reads damage amount and hurt interval from tmx properties.
   /// \param data deserialization data containing water-damage settings.
   void setup(const GameDeserializeData& data);
   /// \brief reports that this mechanism has no local collision rectangle.
   /// \return std::nullopt.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   /// \brief applies one damage tick to the current player.
   void damage();

   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   std::optional<HighResTimePoint> _last_hurt_timepoint;
   HighResDuration _hurt_interval;
   int32_t _hurt_amount{0};
};

#endif  // WATERDAMAGE_H
