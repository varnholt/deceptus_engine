#pragma once

#include <array>
#include <memory>

#include "game/animation/animationpool.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

class InteractionHelp : public GameMechanism, public GameNode
{
public:
   InteractionHelp(GameNode* parent = nullptr);
   std::string_view objectName() const override;
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

   struct HelpElement
   {
      std::unique_ptr<sf::Sprite> _button_sprite;
      std::unique_ptr<sf::Text> _text;
      sf::IntRect _button_rect_keyboard;
      sf::IntRect _button_rect_controller;
   };

   sf::FloatRect _rect_px;
   bool _player_intersected_in_last_frame{false};
   bool _active{false};
   std::shared_ptr<Animation> _animation_show;
   std::shared_ptr<Animation> _animation_hide;
   std::optional<float> _button_alpha;
   static constexpr int32_t button_max_count = 2;
   std::shared_ptr<sf::Texture> _button_texture;
   std::vector<HelpElement> _help_elements;
   sf::Font _font;
};
