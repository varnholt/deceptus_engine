#pragma once

#include <array>
#include <memory>

#include "game/animation/animationpool.h"
#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/level/gamenode.h"

struct TmxObject;

class InteractionHelp : public GameMechanism, public GameNode
{
public:
   InteractionHelp(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void deserialize(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   enum class InteractionType
   {
      Read,
      Examine,
      Use
   };

   sf::FloatRect _rect_px;
   bool _player_intersected_in_last_frame{false};
   bool _active{false};

   std::shared_ptr<Animation> _animation_show;
   std::shared_ptr<Animation> _animation_hide;

   std::optional<float> _button_alpha;
   sf::Sprite _button_sprite;
   std::shared_ptr<sf::Texture> _button_texture;

   sf::Font _font;
   sf::Text _text;

   sf::IntRect _button_rect_keyboard;
   sf::IntRect _button_rect_controller;
};
