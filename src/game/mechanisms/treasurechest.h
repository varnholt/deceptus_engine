#ifndef TREASURECHEST_H
#define TREASURECHEST_H

#include "game/animation.h"
#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

class TreasureChest : public GameMechanism, public GameNode
{
public:
   TreasureChest(GameNode* parent = nullptr);

   void deserialize(const GameDeserializeData& data);

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   enum class Alignment
   {
      Left,
      Right
   };

   sf::FloatRect _rect;
   Alignment _alignment{Alignment::Left};
   std::shared_ptr<sf::Texture> _texture;
   sf::Sprite _sprite;
   std::string _sample_open;

   std::shared_ptr<Animation> _animation_idle_closed;
   std::shared_ptr<Animation> _animation_idle_opening;
   std::shared_ptr<Animation> _animation_idle_open;
};

#endif  // TREASURECHEST_H
