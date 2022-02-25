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

      enum class Mode
      {
         Lever,
         Interval
      };

      SpikeBlock(GameNode* parent = nullptr);

      void deserialize(TmxObject* tmx_object);

      void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;
      void setEnabled(bool enabled) override;

      const sf::IntRect& getPixelRect() const;

private:

      void updateSpriteRect();

      std::shared_ptr<sf::Texture> _texture_map;
      std::shared_ptr<sf::Texture> _normal_map;
      sf::Sprite _sprite;
      sf::IntRect _rectangle;

      static constexpr int32_t _sprite_index_enabled = 16;
      static constexpr int32_t _sprite_index_disabled = 39;

      float _sprite_value = 0.0f;
      int32_t _sprite_index_current = _sprite_index_enabled;
      int32_t _sprite_index_target = _sprite_index_enabled;
      int32_t _tu_tl = 0;
      int32_t _tv_tl = 0;

      Mode _mode = Mode::Lever;
};

