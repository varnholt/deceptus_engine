#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

class OnOffBlock : public GameMechanism, public GameNode
{
public:
    OnOffBlock(GameNode* parent = nullptr);
};

