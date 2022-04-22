#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

class RotatingBlade : public GameMechanism, public GameNode
{
public:
    RotatingBlade(GameNode* parent = nullptr);
};
