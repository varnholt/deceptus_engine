#pragma once

#include <array>
#include <memory>

#include "game/animationpool.h"
#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

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

   std::shared_ptr<sf::Texture> _texture;
   sf::FloatRect _rect_px;
   std::vector<sf::Sprite> _sprites;
   bool _visible = false;
   float _alpha = 0.0f;
   sf::Time _time;

   std::shared_ptr<Animation> _animation_show;
   std::shared_ptr<Animation> _animation_hide;
};
