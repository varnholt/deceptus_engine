#include "effect.h"

#include <vector>
#include <cmath>


class Pixelate : public Effect
{
public:

    Pixelate();

    bool onLoad();
    void onUpdate(const sf::Time& /*time*/, float x, float y);
    void onDraw(sf::RenderTarget& target, sf::RenderStates states) const;

private:

    std::shared_ptr<sf::Texture> mTexture;
    sf::Sprite mSprite;
    sf::Shader mShader;
};

