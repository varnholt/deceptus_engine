#ifndef ENEMYWALL_H
#define ENEMYWALL_H

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class EnemyWall : public GameMechanism, public GameNode
{
public:
   EnemyWall(GameNode* parent = nullptr);

   void setup(const GameDeserializeData& data);
   void update(const sf::Time& dt) override;
   void setEnabled(bool enabled) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   const sf::FloatRect& getPixelRect() const;

private:
   // rendering
   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   sf::Sprite _sprite;
   sf::FloatRect _rectangle;

   // physics
   b2Body* _body = nullptr;
   b2Vec2 _position_b2d;
   sf::Vector2f _position_sfml;
   b2PolygonShape _shape_bounds;
};

#endif  // ENEMYWALL_H
