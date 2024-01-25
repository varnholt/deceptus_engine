#ifndef INFOOVERLAY_H
#define INFOOVERLAY_H

#include <SFML/Graphics.hpp>
#include <chrono>

#include "game/gamedeserializedata.h"
#include "game/gamemechanism.h"
#include "game/gamenode.h"

class InfoOverlay : public GameMechanism, public GameNode
{
public:
   InfoOverlay(GameNode* parent = nullptr);

   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::shared_ptr<InfoOverlay> setup(GameNode* parent, const GameDeserializeData& data);

private:
   struct Settings
   {
      using FloatSeconds = std::chrono::duration<float>;
      FloatSeconds _fade_in_duration;
      FloatSeconds _show_duration;
      FloatSeconds _fade_out_duration;
   };

   sf::FloatRect _rect;
   sf::IntRect _texture_rect;
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::Time _elapsed;
};

#endif  // INFOOVERLAY_H
