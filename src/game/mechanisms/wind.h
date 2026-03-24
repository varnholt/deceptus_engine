#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <memory>

/// \brief applies directional force to the player while inside a rectangular wind area.
class Wind : public GameMechanism, public GameNode
{
public:
   /// \brief creates a wind mechanism instance.
   /// \param parent owning game node in the scene graph.
   explicit Wind(GameNode* parent = nullptr);
   ~Wind() override = default;

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "Wind".
   std::string_view objectName() const override;

   /// \brief applies configured force to the player body when inside the wind area.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief draws debug visuals for wind if implemented, currently no-op.
   /// \param target render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief returns the wind area rectangle in pixel space.
   /// \return rectangular area used for force application checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the configured wind force vector.
   /// \return direction vector applied to the player's box2d body.
   const sf::Vector2f& getDirection() const;

   /// \brief creates a wind instance from tmx area and direction properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data with bounds and direction components.
   /// \return configured wind mechanism.
   static std::shared_ptr<Wind> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   sf::Vector2f _direction{0.f, 0.f};
   sf::FloatRect _area;
};
