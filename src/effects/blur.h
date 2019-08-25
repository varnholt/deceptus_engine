#include "effect.h"
#include <SFML/Graphics.hpp>

#include <vector>
#include <cmath>


class WaveBlur : public Effect
{
public:

    WaveBlur();
    bool onLoad();
    void onUpdate(const sf::Time& time, float x, float y);
    void onDraw(sf::RenderTarget& target, sf::RenderStates states) const;


private:

    sf::Text mText;
    sf::Shader mShader;
    sf::Font mFont;
};

