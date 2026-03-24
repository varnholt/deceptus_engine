#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include "SFML/Graphics.hpp"

#include <functional>
#include <memory>
#include <vector>

struct TmxObject;

/// \brief wall-clock checkpoint that updates player respawn state when touched.
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

   /// \brief creates a checkpoint and initializes checkpoint audio settings.
   /// \param parent parent node in the scene graph.
   Checkpoint(GameNode* parent = nullptr);
   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "Checkpoint".
   std::string_view objectName() const override;

   /// \brief finds a checkpoint by index in a mechanism list.
   /// \param index checkpoint index to search for.
   /// \param checkpoints mechanism list that may contain checkpoint instances.
   /// \return matching checkpoint, or nullptr when not found.
   static std::shared_ptr<Checkpoint> getCheckpoint(int32_t index, const std::vector<std::shared_ptr<GameMechanism>>& checkpoints);
   /// \brief creates and initializes a checkpoint from tmx object data.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with object properties and paths.
   /// \return initialized checkpoint instance.
   static std::shared_ptr<Checkpoint> deserialize(GameNode* parent, const GameDeserializeData& data);

   /// \brief draws the checkpoint sprite.
   /// \param target render target.
   /// \param normal normal render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   /// \brief checks player overlap and advances checkpoint animation when active.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;
   /// \brief returns the checkpoint trigger rectangle in pixel coordinates.
   /// \return checkpoint bounds used for overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief marks this checkpoint as reached and runs registered save-state callbacks.
   void reached();
   /// \brief registers a callback executed when the checkpoint is reached.
   /// \param cb callback to append.
   void addCallback(CheckpointCallback);
   /// \brief returns the player respawn position for this checkpoint.
   /// \return spawn position in pixel coordinates.
   sf::Vector2f spawnPoint() const;
   /// \brief returns the checkpoint index used in save data.
   /// \return checkpoint index.
   int32_t getIndex() const;
   /// \brief updates sprite frame selection for inactive, activating, or active states.
   /// \param dt_s elapsed time in seconds used to advance animation.
   void updateSpriteRect(float dt_s = 0.0f);

private:
   int32_t _index = 0;
   std::string _name;
   sf::FloatRect _rect;
   bool _reached = false;
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   State _state{State::Inactive};
   float _sprite_index{0.0f};
   sf::Vector2f _spawn_point;
   bool _tick_played = false;
   bool _tock_played = false;

   std::vector<CheckpointCallback> _callbacks;
};
