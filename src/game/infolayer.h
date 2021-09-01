#pragma once

#include "animation.h"
#include "bitmapfont.h"

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>


class InfoLayer
{
public:

   InfoLayer();

   void update(const sf::Time& dt);
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void drawDebugInfo(sf::RenderTarget& window);
   void drawConsole(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);

   void setLoading(bool loading);

private:

   void playHeartAnimation();
   void drawHeartAnimation(sf::RenderTarget& window);

   BitmapFont _font;

   bool _loading = false;
   sf::Time _show_time;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   Animation _heart_animation;
};

