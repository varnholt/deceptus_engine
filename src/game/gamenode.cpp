#include "gamenode.h"

#include <iostream>


GameNode::GameNode(GameNode* parent)
 : _parent(parent)
{
    if (parent)
    {
        parent->_children.push_back(this);
    }
}


GameNode* GameNode::getParent() const
{
    return _parent;
}


void GameNode::dump(int32_t depth)
{
    std::cout << "+";

    for (auto i = 0; i < depth; i++)
    {
        std::cout << "--";
    }

    std::cout << " " << this << " (" << _name << ") " << "[" << _children.size() << "]" << std::endl;

    for (auto& c : _children)
    {
        c->dump(depth + 1);
    }
}


void GameNode::setName(const std::string& name)
{
    _name = name;
}

