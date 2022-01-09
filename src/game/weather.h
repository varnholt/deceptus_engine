#pragma once

#include "gamenode.h"
#include "game/gamemechanism.h"
#include "overlays/rainoverlay.h"
#include "overlays/thunderstormoverlay.h"

#include <memory>
#include <SFML/Graphics.hpp>

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

      static std::shared_ptr<Weather> deserialize(TmxObject* tmx_object);


   private:

      std::shared_ptr<WeatherOverlay> _overlay;
      sf::IntRect _rect;
};

