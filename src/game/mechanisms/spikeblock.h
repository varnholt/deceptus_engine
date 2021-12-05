#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include <memory>

// Spikeblock
//
// They work the same as the other spikes but the collision box is a full 24x24 tile.
// You can place it anywhere in the level, even in mid-air.
// They can be switched on/off by a lever, work as a trap spike, or interval spike.
// Similar Example : https://i.ytimg.com/vi/hOo858bd1L0/maxresdefault.jpg.
//
// modes
// - interval
// - switchable

struct TmxObject;


class SpikeBlock : public GameMechanism, public GameNode
{
   public:
      SpikeBlock(GameNode* parent = nullptr);

      void deserialize(TmxObject* tmx_object);

      void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
      void update(const sf::Time& dt);

   private:
      std::shared_ptr<sf::Texture> _texture_map;
      std::shared_ptr<sf::Texture> _normal_map;
      sf::Sprite _sprite;
      sf::IntRect _rectangle;

};

