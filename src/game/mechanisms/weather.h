#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/layers/rainoverlay.h"
#include "game/layers/thunderstormoverlay.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

struct TmxObject;

/// \brief drives area-based weather overlays such as rain and thunderstorms.
class Weather : public GameMechanism, public GameNode
{
public:
   /// \brief creates a weather mechanism instance.
   /// \param parent owning game node in the scene graph.
   Weather(GameNode* parent = nullptr);
   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Weather".
   std::string_view objectName() const override;
   /// \brief draws the configured weather overlay when player, room, and delay conditions match.
   /// \param target render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   /// \brief updates start-delay logic and advances overlay animation while active.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns the weather activation rectangle in pixel space.
   /// \return area that gates weather updates and drawing.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates rain or thunderstorm weather from object name and properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with bounds and weather settings.
   /// \return configured weather instance.
   static std::shared_ptr<Weather> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   /// \brief updates delayed-start state based on intersection and room match transitions.
   /// \param dt elapsed frame time.
   /// \param intersects true when the player intersects the weather rectangle.
   void updateWaitDelay(const sf::Time& dt, bool intersects);
   /// \brief checks whether room restrictions allow this weather to run.
   /// \return true when room filtering is disabled or the current room is listed.
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
