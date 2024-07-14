#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/layers/rainoverlay.h"
#include "game/layers/thunderstormoverlay.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

struct TmxObject;

/*! \brief A base class for all sorts of weather mechanisms.
 *         So far 'thunderstorm' and 'rain' is supported.
 *
 *  Weather works like any other mechanism; it has a deserialize function to load properties,
 *  an update function to drive the weather behavior and a draw function to paint it.
 */
class Weather : public GameMechanism, public GameNode
{
public:
   Weather(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::shared_ptr<Weather> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   void updateWaitDelay(const sf::Time& dt, bool intersects);
   bool matchesRoom() const;

   std::shared_ptr<WeatherOverlay> _overlay;
   sf::FloatRect _rect;
   bool _limit_effect_to_room{false};
   std::optional<bool> _draw_allowed_in_previous_frame;

   using FloatSeconds = std::chrono::duration<float>;

   std::optional<FloatSeconds> _effect_start_delay;
   FloatSeconds _elapsed_since_intersect;
   bool _wait_until_start_delay_elapsed{false};
};
