
#ifndef BLOCKINGRECT_H
#define BLOCKINGRECT_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief represents a static solid rectangle that blocks movement and optionally renders custom textures.
class BlockingRect : public GameMechanism, public GameNode
{
public:
   /// \brief creates a blocking rectangle node.
   /// \param parent parent node in the scene graph.
   BlockingRect(GameNode* parent = nullptr);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "BlockingRect".
   std::string_view objectName() const override;

   /// \brief initializes rectangle geometry, optional textures, and a static box2d collider from tmx data.
   /// \param data deserialize context containing object properties and physics world.
   void setup(const GameDeserializeData& data);
   /// \brief draws the configured texture and normal map when the rectangle is enabled.
   /// \param target color render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   /// \brief updates runtime logic.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief enables or disables the underlying box2d body.
   /// \param enabled true to keep the blocker collidable.
   void setEnabled(bool enabled) override;
   /// \brief returns the blocking rectangle in pixel coordinates.
   /// \return rectangle bounds used for culling and overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the configured pixel rectangle.
   /// \return rectangle occupied by this blocking mechanism.
   const sf::FloatRect& getPixelRect() const;

private:
   // rendering
   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rectangle;

   // physics
   b2Body* _body = nullptr;
   b2Vec2 _position_b2d;
   sf::Vector2f _position_sfml;
   b2PolygonShape _shape_bounds;
};

#endif  // BLOCKINGRECT_H
