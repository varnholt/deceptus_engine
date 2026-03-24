#ifndef ENEMYWALL_H
#define ENEMYWALL_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief creates a static collision barrier that only enemies collide with.
class EnemyWall : public GameMechanism, public GameNode
{
public:
   /// \brief creates an enemy wall mechanism.
   /// \param parent parent node in the scene graph.
   EnemyWall(GameNode* parent = nullptr);
   /// \brief returns the mechanism registry name.
   /// \return string view containing `EnemyWall`.
   std::string_view objectName() const override;

   /// \brief initializes rectangle data and creates a static box2d body with enemy-only filters.
   /// \param data deserialize context with TMX object and physics world.
   void setup(const GameDeserializeData& data);
   /// \brief updates the mechanism state.
   ///
   /// this mechanism currently has no per-frame behavior.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief enables or disables collision by toggling the box2d body.
   /// \param enabled true to enable collision, false to disable it.
   void setEnabled(bool enabled) override;
   /// \brief returns the configured wall rectangle.
   /// \return wall rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the wall rectangle in pixel coordinates.
   /// \return enemy-wall rectangle in pixels.
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

#endif  // ENEMYWALL_H
