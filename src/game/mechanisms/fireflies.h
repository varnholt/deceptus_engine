
#ifndef FIREFLIES_H
#define FIREFLIES_H

#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

class Fireflies : public GameMechanism, public GameNode
{
public:
   struct Firefly
   {
      void update(const sf::Time& dt);

      sf::Vector2f _position;
      sf::Sprite _sprite;
   };

   Fireflies(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void deserialize(const GameDeserializeData& data);

private:
   sf::FloatRect _rect;
   std::vector<Firefly> _fireflies;
   std::shared_ptr<sf::Texture> _texture;
};

#endif // FIREFLIES_H
