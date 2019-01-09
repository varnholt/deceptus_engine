#include "gamenode.h"

GameNode::GameNode(GameNode *parent)
 : mParent(parent)
{
}


GameNode::~GameNode()
{
}


GameNode *GameNode::getParent() const
{
   return mParent;
}

