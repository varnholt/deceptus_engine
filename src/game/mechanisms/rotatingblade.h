#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include "gamedeserializedata.h"


class RotatingBlade : public GameMechanism, public GameNode
{
public:
    RotatingBlade(GameNode* parent = nullptr);

    void setup(const GameDeserializeData& data);
    void update(const sf::Time& dt);
    void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
    void setEnabled(bool enabled) override;

private:
    float _angle = 0.0f;
    float _velocity = 0.0f;
    float _direction = 1.0f;

    std::shared_ptr<sf::Texture> _texture_map;
    std::shared_ptr<sf::Texture> _normal_map;
    sf::Sprite _sprite;
    sf::IntRect _rectangle;
};
