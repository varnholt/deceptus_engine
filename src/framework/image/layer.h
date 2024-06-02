#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

struct Layer : public sf::Drawable
{
   Layer() = default;

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

   void show();
   void hide();

   bool _visible = true;

   std::string _name;
   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Sprite> _sprite;
};
