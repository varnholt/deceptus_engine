#pragma once

#include "game/gamemechanism.h"
#include "gamedeserializedata.h"
#include "gamenode.h"
#include "overlays/rainoverlay.h"
#include "overlays/thunderstormoverlay.h"

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
   std::shared_ptr<WeatherOverlay> _overlay;
   sf::FloatRect _rect;
};
