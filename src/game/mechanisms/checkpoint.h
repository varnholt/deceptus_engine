#pragma once

#include "gamemechanism.h"

#include "SFML/Graphics.hpp"

#include <functional>
#include <vector>


struct TmxObject;


class Checkpoint : public GameMechanism
{

public:

   using CheckpointCallback = std::function<void(void)>;

   Checkpoint() = default;

   static std::shared_ptr<Checkpoint> getCheckpoint(
      uint32_t index,
      const std::vector<std::shared_ptr<GameMechanism>>& checkpoints
   );

   static std::shared_ptr<Checkpoint> add(TmxObject*);

   void update(const sf::Time& dt);

   void reached();
   void addCallback(CheckpointCallback);
   sf::Vector2i calcCenter() const;
   uint32_t getIndex() const;

private:

   uint32_t _index = 0;
   std::string _name;
   sf::IntRect _rect;
   bool _reached = false;

   std::vector<CheckpointCallback> _callbacks;
};

