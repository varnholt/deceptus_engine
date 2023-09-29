#pragma once

#include <array>
#include <memory>

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

struct TmxObject;

class ControllerHelp : public GameMechanism, public GameNode
{
public:
   ControllerHelp(GameNode* parent = nullptr);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;
   void deserialize(const GameDeserializeData& data);
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect_px;
   sf::Vector2f _rect_center;
   std::vector<sf::Sprite> _sprites;
   sf::Sprite _background;
   bool _visible = false;
   float _alpha = 0.0f;
   sf::Time _time;
};
