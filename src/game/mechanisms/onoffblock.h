#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <deque>
#include <memory>

struct TmxObject;

class OnOffBlock : public GameMechanism, public GameNode
{
public:
   enum class Mode
   {
      Lever,
      Interval
   };

   OnOffBlock(GameNode* parent = nullptr);
   std::string_view objectName() const override;

   void setup(const GameDeserializeData& data);

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void setEnabled(bool enabled) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   const sf::FloatRect& getPixelRect() const;

private:
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
