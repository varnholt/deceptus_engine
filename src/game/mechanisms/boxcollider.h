#pragma once

#include <box2d/box2d.h>
#include <memory>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class BoxCollider : public GameMechanism, public GameNode
{
public:
   BoxCollider(GameNode* node);
   std::string_view objectName() const override;
   void setup(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   void setupBody(const std::shared_ptr<b2World>& world);
   b2Body* _body = nullptr;
   sf::Vector2f _size;
   sf::FloatRect _rect;
};
