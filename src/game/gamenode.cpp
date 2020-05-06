#include "gamenode.h"

#include <iostream>


GameNode::GameNode(GameNode* parent)
 : mParent(parent)
{
    if (parent)
    {
        parent->mChildren.push_back(this);
    }
}


GameNode* GameNode::getParent() const
{
    return mParent;
}


void GameNode::dump(int32_t depth)
{
    std::cout << "+";

    for (auto i = 0; i < depth; i++)
    {
        std::cout << "--";
    }

    std::cout << " " << this << " (" << mName << ") " << "[" << mChildren.size() << "]" << std::endl;

    for (auto& c : mChildren)
    {
        c->dump(depth + 1);
    }
}


void GameNode::setName(const std::string& name)
{
    mName = name;
}

