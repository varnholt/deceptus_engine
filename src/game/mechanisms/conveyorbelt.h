#pragma once

#include <filesystem>

#include "box2d/box2d.h"
#include "SFML/Graphics.hpp"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief moving belt mechanism that pushes colliding bodies horizontally.
class ConveyorBelt : public FixtureNode, public GameMechanism
{
public:
   /// \brief creates belt geometry, sprites, and velocity from tmx data.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with tmx object and physics world.
   ConveyorBelt(GameNode* parent, const GameDeserializeData& data);
   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "ConveyorBelt".
   std::string_view objectName() const override;

   /// \brief returns the static belt body used for contact processing.
   /// \return non-owning pointer to the belt body.
   b2Body* getBody() const;
   /// \brief returns current belt velocity contribution.
   /// \return horizontal velocity added to bodies on contact.
   float getVelocity() const;
   /// \brief sets belt velocity and updates visual movement direction.
   /// \param velocity horizontal velocity to apply.
   void setVelocity(float velocity);

   /// \brief draws belt tiles.
   /// \param color color render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   /// \brief updates belt animation timing and lever lag fade.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief enables or disables belt behavior with smooth lever-lag transition.
   /// \param enabled true to run the belt.
   void setEnabled(bool enabled) override;
   /// \brief returns belt bounds in pixel coordinates.
   /// \return rectangle used for culling and queries.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns belt rectangle in pixel coordinates.
   /// \return belt bounds.
   sf::FloatRect getPixelRect() const;

   /// \brief clears per-frame contact bookkeeping and resets player belt state.
   static void resetBeltState();
   /// \brief processes one box2d contact to apply belt velocity to supported bodies.
   /// \param contact contact pair to inspect.
   static void processContact(b2Contact* contact);
   /// \brief applies belt behavior when one fixture belongs to a conveyor belt.
   /// \param fixtureNode fixture node associated with one contact fixture.
   /// \param collidingBody body of the opposite contact fixture.
   static void processFixtureNode(FixtureNode* fixtureNode, b2Body* collidingBody);

   /// \brief updates scrolling belt tile texture offsets based on elapsed time and direction.
   void updateSprite();

private:
   b2Body* _body = nullptr;
   b2Vec2 _position_b2d;
   sf::Vector2f _position_sfml;
   b2PolygonShape _shape;
   sf::FloatRect _belt_pixel_rect;
   sf::FloatRect _arrow_pixel_rect;
   float _elapsed = 0.0f;
   bool _points_right = true;
   float _lever_lag = 1.0f;

   std::shared_ptr<sf::Texture> _texture;
   std::vector<sf::Sprite> _belt_sprites;
   std::vector<sf::Sprite> _arrow_sprites;

   // bool mActive = true;
   float _velocity = -0.2f;

   static std::vector<b2Body*> __bodies_on_belt;
};
