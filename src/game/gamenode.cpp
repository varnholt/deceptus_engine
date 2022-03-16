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


GameNode::~GameNode()
{
   // unlink children
   for (auto& c : _children)
   {
      c->_parent = nullptr;
   }

   // unlink from parent
   if (_parent)
   {
      std::erase_if(_parent->_children, [this](auto ptr){return ptr == this;});
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

    // if (_class_name.empty())
    // {
    //    std::cerr << "empty class name" << std::endl;
    // }
    //
    // if (_object_name.empty())
    // {
    //    std::cerr << "empty object name" << std::endl;
    // }

    std::cout << " " << this << " " << _class_name << ", object: '" << _object_id << "' [" << _children.size() << "]" << std::endl;

    for (auto& c : _children)
    {
        c->dump(depth + 1);
    }
}


void GameNode::setClassName(const std::string& name)
{
   _class_name = name;
}


const std::string& GameNode::getObjectId() const
{
   return _object_id;
}


void GameNode::setObjectId(const std::string& newObject_name)
{
   _object_id = newObject_name;
}

