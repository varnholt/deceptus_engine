#pragma once

#include <array>

#include "gamenode.h"
#include "gamemechanism.h"

struct TmxObject;

class ControllerHelp : public GameMechanism, public GameNode
{
   public:

      ControllerHelp(GameNode* parent = nullptr);
      void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
      void update(const sf::Time& dt) override;
      void deserialize(TmxObject* tmx_object);

   private:

      std::shared_ptr<sf::Texture> _texture;
      sf::IntRect _rect_px;
      sf::Vector2f _rect_center;
      std::vector<sf::Sprite> _sprites;
      bool _visible = false;
      sf::Time _time;
};

