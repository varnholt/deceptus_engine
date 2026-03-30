#pragma once

#include "box2d/box2d.h"
#include <memory>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief static rectangular collider used as a solid obstacle in the physics world.
class BoxCollider : public GameMechanism, public GameNode
{
public:
   /// \brief creates an empty box collider node.
   /// \param node parent node in the scene graph.
   BoxCollider(GameNode* node);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "BoxCollider".
   std::string_view objectName() const override;

   /// \brief initializes collider size, bounds, chunk data, and static body from tmx data.
   /// \param data deserialize context with object geometry and world pointer.
   void setup(const GameDeserializeData& data);

   /// \brief returns the collider bounds in pixel coordinates.
   /// \return rectangle used for culling and spatial queries.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   /// \brief creates the static box2d obstacle body and fixture.
   /// \param world physics world that owns the created body.
   void setupBody(const std::shared_ptr<b2World>& world);
   b2Body* _body = nullptr;
   sf::Vector2f _size;
   sf::FloatRect _rect;
};
