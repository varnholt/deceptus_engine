#pragma once

#include <SFML/Graphics.hpp>
#include <map>

#include "../../src/framework/image/layer.h"

class TestMechanism
{
public:
   TestMechanism();
   virtual ~TestMechanism() = default;
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   virtual void update(const sf::Time& dt);

private:
   void load();

   sf::RectangleShape _rectangle_;
   sf::CircleShape _origin_shape;

   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   std::array<std::shared_ptr<Layer>, 4> _pa;
   // std::array<float, 4> _angles;
   float _elapsed{0.0f};
   sf::Vector2f _origin;
};
