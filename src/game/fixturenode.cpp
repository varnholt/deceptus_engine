// base
#include "fixturenode.h"



FixtureNode::FixtureNode(GameNode* parent)
 : GameNode(parent)
{
    setName(typeid(FixtureNode).name());
}


ObjectType FixtureNode::getType() const
{
   return _type;
}


void FixtureNode::setType(const ObjectType& type)
{
   _type = type;
}


void FixtureNode::setFlag(const std::string& flag, bool value)
{
   _flags[flag]=value;
}


bool FixtureNode::hasFlag(const std::string& flag)
{
   bool value = _flags[flag];
   return value;
}


void FixtureNode::setProperty(const std::string& key, const Variant& value)
{
    _properties[key] = value;
}


FixtureNode::Variant FixtureNode::getProperty(const std::string& key) const
{
   return _properties.find(key)->second;
}


void FixtureNode::collisionWithPlayer()
{
   _collision_callback();
}

void FixtureNode::setCollisionCallback(const CollisionCallback& collisionCallback)
{
   _collision_callback = collisionCallback;
}

