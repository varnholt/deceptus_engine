#pragma once

#include <array>
#include <memory>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

class ControllerHelp : public GameMechanism, public GameNode
{
public:
   ControllerHelp(GameNode* parent = nullptr);
   std::string_view objectName() const override;
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void deserialize(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect_px;
   sf::Vector2f _rect_center;

   std::vector<sf::Sprite> _sprites;
   std::vector<sf::IntRect> _sprite_rects_keyboard;
   std::vector<sf::IntRect> _sprite_rects_controller;

   std::unique_ptr<sf::Sprite> _background;
   float _alpha = 0.0f;
   sf::Time _time;
};
