#pragma once

#include "weather.h"


struct TmxObject;

class ThunderstormOverlay : public WeatherOverlay
{

public:

   ThunderstormOverlay();

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   void setRect(const sf::FloatRect& newRect);


private:

   //sf::RectangleShape _shape;
   sf::Sprite _sprite;
   sf::FloatRect _rect;
   float _value = 0.0f;
   float _time_s = 0.0f;
};

