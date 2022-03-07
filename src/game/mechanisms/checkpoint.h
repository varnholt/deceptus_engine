#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

#include <functional>
#include <memory>
#include <vector>


struct TmxObject;


class Checkpoint : public GameMechanism, public GameNode
{

public:

   using CheckpointCallback = std::function<void(void)>;

   Checkpoint(GameNode* parent = nullptr);

   static std::shared_ptr<Checkpoint> getCheckpoint(
      uint32_t index,
      const std::vector<std::shared_ptr<GameMechanism>>& checkpoints
   );

   static std::shared_ptr<Checkpoint> deserialize(GameNode* parent, TmxObject*);

   void draw(sf::RenderTarget &target, sf::RenderTarget &normal) override;
   void update(const sf::Time& dt) override;

   void reached();
   void addCallback(CheckpointCallback);
   sf::Vector2i calcCenter() const;
   uint32_t getIndex() const;
   void updateSpriteRect();

private:

   uint32_t _index = 0;
   std::string _name;
   sf::IntRect _rect;
   bool _reached = false;
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;

   std::vector<CheckpointCallback> _callbacks;
};

