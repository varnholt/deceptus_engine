#pragma once

#include "box2d/box2d.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "SFML/Graphics.hpp"

/// \brief spring mechanism that launches the player by applying an impulse when activated.
class Bouncer : public FixtureNode, public GameMechanism
{
public:
   /// \brief creates a bouncer fixture, sprite, and launch settings from tmx data.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with tmx object and world.
   Bouncer(GameNode* parent, const GameDeserializeData& data);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "Bouncer".
   std::string_view objectName() const override;

   /// \brief preloads the bounce sound sample.
   void preload() override;

   /// \brief draws the animated bouncer sprite.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief updates player proximity state and animation frame.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the bouncer interaction rectangle in pixel coordinates.
   /// \return bouncer bounds used for interaction checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief checks whether the player is currently close enough to use this bouncer.
   /// \return true when the enlarged player rect intersects the bouncer rect.
   bool isPlayerAtBouncer();

   /// \brief applies an impulse to the player and restarts the activation animation when cooldown allows.
   void activate();

   /// \brief returns the box2d body used for collision and sensor callbacks.
   /// \return non-owning pointer to the bouncer body.
   b2Body* getBody() const;

private:
   /// \brief refreshes whether the player is standing at the bouncer.
   void updatePlayerAtBouncer();

   Alignment _alignment = Alignment::PointsUp;
   b2Body* _body = nullptr;
   b2Vec2 _position_b2d;
   sf::Vector2f _position_sfml;
   b2PolygonShape _shape_bounds;
   b2PolygonShape _shape_sensor;

   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rect;
   sf::Time _activation_time;
   bool _player_at_bouncer = false;
   float _force_value = 0.6f;
   std::optional<int32_t> _previous_step;
};
