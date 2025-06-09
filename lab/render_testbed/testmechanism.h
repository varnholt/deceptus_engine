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
   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
};
