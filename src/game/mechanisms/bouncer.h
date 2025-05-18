#pragma once

#include <box2d/box2d.h>
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxobjectgroup.h"
#include "game/constants.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "SFML/Graphics.hpp"

class Bouncer : public FixtureNode, public GameMechanism
{
public:
   Bouncer(GameNode* parent, const GameDeserializeData& data);

   void preload() override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   bool isPlayerAtBouncer();

   void activate();
   b2Body* getBody() const;

private:
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
