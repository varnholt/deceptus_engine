#ifndef TREASURECHEST_H
#define TREASURECHEST_H

#include "game/animation/animation.h"
#include "game/effects/spawneffect.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief interactive chest that opens with optional key requirements and can spawn rewards.
class TreasureChest : public GameMechanism, public GameNode
{
public:
   /// \brief creates a treasure chest mechanism instance.
   /// \param parent owning game node in the scene graph.
   TreasureChest(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "TreasureChest".
   std::string_view objectName() const override;

   /// \brief loads chest visuals, sounds, key requirement, and reward settings from tmx data.
   /// \param data deserialization data containing chest configuration properties.
   void deserialize(const GameDeserializeData& data);

   /// \brief draws the current chest animation and active spawn effect.
   /// \param target render target.
   /// \param /*normal*/ normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;

   /// \brief handles interaction, state transitions, and reward spawning.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief reports that this mechanism does not expose a gameplay bounding box.
   /// \return std::nullopt.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   enum class Alignment
   {
      Left,
      Right
   };

   enum class State
   {
      Closed,
      Opening,
      Open
   };

   /// \brief checks whether the player satisfies the configured key requirement.
   /// \return true when no key is required or when the required inventory item is present.
   bool playerHasRequiredKey() const;

   sf::FloatRect _rect;
   Alignment _alignment{Alignment::Left};
   std::shared_ptr<sf::Texture> _texture;
   std::unique_ptr<sf::Sprite> _sprite;
   std::string _sample_open;
   std::string _sample_locked;
   State _state{State::Closed};
   std::optional<std::string> _spawn_extra;
   std::optional<std::string> _item_required;

   std::shared_ptr<Animation> _animation_idle_closed;
   std::shared_ptr<Animation> _animation_opening;
   std::shared_ptr<Animation> _animation_idle_open;
   std::unique_ptr<SpawnEffect> _spawn_effect;
};

#endif  // TREASURECHEST_H
