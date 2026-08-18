#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <deque>
#include <memory>

struct TmxObject;

/// \brief controls a solid block that switches collision and visuals on lever or interval timing.
class OnOffBlock : public GameMechanism, public GameNode
{
public:
   enum class Mode
   {
      Lever,
      Interval
   };

   /// \brief creates an on-off block mechanism.
   /// \param parent parent node in the scene graph.
   OnOffBlock(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `OnOffBlock`.
   std::string_view objectName() const override;

   /// \brief initializes sprite, timing mode, inversion, and static box2d body from TMX data.
   /// \param data deserialize context containing TMX object and world.
   void setup(const GameDeserializeData& data);

   /// \brief draws the current on-off block sprite frame.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief draws the current on-off block sprite frame with explicit render states (used in WASM to carry the level view).
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override;
   using GameMechanism::draw;

   /// \brief updates interval toggling, queued state changes, and transition animation.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief queues a target enabled state and applies inversion when configured.
   /// \param enabled requested enabled state before inversion rules are applied.
   void setEnabled(bool enabled) override;

   /// \brief returns the block rectangle in pixel coordinates.
   /// \return block rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief returns the block rectangle used for intersection checks.
   /// \return block rectangle in pixel coordinates.
   const sf::FloatRect& getPixelRect() const;

private:
   /// \brief updates sprite texture coordinates from the current animation index.
   void updateSpriteRect();

   std::shared_ptr<sf::Texture> _texture_map;
   std::shared_ptr<sf::Texture> _normal_map;
   std::unique_ptr<sf::Sprite> _sprite;
   sf::FloatRect _rectangle;

   static constexpr int32_t _sprite_index_enabled = 14;
   static constexpr int32_t _sprite_index_disabled = 31;

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
   bool _inverted{false};

   // b2d
   b2Body* _body = nullptr;
   b2Fixture* _fixture = nullptr;
   b2Vec2 _position_m;
   b2PolygonShape _shape;
};
