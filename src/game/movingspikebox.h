#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

class MovingSpikeBox: public GameMechanism, public GameNode
{
    public:
        MovingSpikeBox(GameNode* parent);

        void draw(sf::RenderTarget& window) override;
        void update(const sf::Time& dt) override;

};

