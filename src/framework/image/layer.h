#pragma once

#include <memory>
#include <SFML/Graphics.hpp>

struct Layer : public sf::Drawable
{
   Layer() = default;

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   void show();
   void hide();

   bool _visible = true;

   //sf::Vector2f mOffset;
   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Sprite> _sprite;
};

