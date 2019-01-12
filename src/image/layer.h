#pragma once

#include <memory>
#include <SFML/Graphics.hpp>

struct Layer : public sf::Drawable
{
   Layer() = default;

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   void show();
   void hide();

   bool mVisible = true;

   sf::Vector2f mOffset;
   std::shared_ptr<sf::Texture> mTexture;
   std::shared_ptr<sf::Sprite> mSprite;
};

