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

    std::cout << " " << this << " " << _class_name << ", object: '" << _object_name << "' [" << _children.size() << "]" << std::endl;

    for (auto& c : _children)
    {
        c->dump(depth + 1);
    }
}


void GameNode::setClassName(const std::string& name)
{
   _class_name = name;
}


const std::string& GameNode::getObjectName() const
{
   return _object_name;
}


void GameNode::setObjectName(const std::string& newObject_name)
{
   _object_name = newObject_name;
}

