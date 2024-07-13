#pragma once

#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/level/gamenode.h"

#include "SFML/Graphics.hpp"

#include <functional>
#include <memory>
#include <vector>

struct TmxObject;

class Checkpoint : public GameMechanism, public GameNode
{
public:
   enum class State
   {
      Inactive,
      Activating,
      Active
   };

   using CheckpointCallback = std::function<void(void)>;

   Checkpoint(GameNode* parent = nullptr);

   static std::shared_ptr<Checkpoint> getCheckpoint(int32_t index, const std::vector<std::shared_ptr<GameMechanism>>& checkpoints);
   static std::shared_ptr<Checkpoint> deserialize(GameNode* parent, const GameDeserializeData& data);

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void reached();
   void addCallback(CheckpointCallback);
   sf::Vector2f spawnPoint() const;
   int32_t getIndex() const;
   void updateSpriteRect(float dt_s = 0.0f);

private:
   int32_t _index = 0;
   std::string _name;
   sf::FloatRect _rect;
   bool _reached = false;
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   State _state{State::Inactive};
   float _sprite_index{0.0f};
   sf::Vector2f _spawn_point;
   bool _tick_played = false;
   bool _tock_played = false;

   std::vector<CheckpointCallback> _callbacks;
};
