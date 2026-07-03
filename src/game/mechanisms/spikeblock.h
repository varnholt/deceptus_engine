#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <deque>
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

/// \brief animated spike tile that can be lever-controlled or run on an interval.
class SpikeBlock : public GameMechanism, public GameNode
{
public:
   /// \brief activation mode for the spike block.
   enum class Mode
   {
      Lever,
      Interval
   };

   /// \brief creates a spike block mechanism.
   /// \param parent owning game node in the scene graph.
   SpikeBlock(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "SpikeBlock".
   std::string_view objectName() const override;

   /// \brief initializes sprite, bounds, mode, and timing from tmx data.
   /// \param data deserialization data for this spike block object.
   void setup(const GameDeserializeData& data);

   /// \brief draws the current spike block animation frame.
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws the current spike block animation frame with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief advances animation state, interval toggles, and player damage checks.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief queues a target enabled state for animated transitions.
   /// \param enabled true to animate toward the extended state.
   void setEnabled(bool enabled) override;

   /// \brief returns the block rectangle in pixel space.
   /// \return area used for chunk activation and collision damage tests.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the spike block rectangle in pixel space.
   /// \return rectangle occupied by the block.
   const sf::FloatRect& getPixelRect() const;

private:
   /// \brief updates sprite texture coordinates from the current frame index.
   void updateSpriteRect();

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rectangle;

   static constexpr int32_t _sprite_index_enabled = 32;
   static constexpr int32_t _sprite_index_disabled = 39;
   static constexpr int32_t _sprite_index_deadly_min = _sprite_index_enabled - 3;
   static constexpr int32_t _sprite_index_deadly_max = _sprite_index_enabled + 3;

   float _sprite_value = _sprite_index_enabled;
   int32_t _sprite_index_current = _sprite_index_enabled;
   int32_t _sprite_index_target = _sprite_index_enabled;
   std::deque<bool> _target_states;
   int32_t _tu_tl = 0;
   int32_t _tv_tl = 0;

   Mode _mode = Mode::Lever;
   sf::Time _elapsed;
   int32_t _time_on_ms = 4000;
   int32_t _time_off_ms = 3000;
};
