// base
#include "fixturenode.h"



FixtureNode::FixtureNode(GameNode* parent)
 : GameNode(parent)
{
}


FixtureNode::~FixtureNode()
{
}


ObjectType FixtureNode::getType() const
{
   return mType;
}


void FixtureNode::setType(const ObjectType &type)
{
   mType = type;
}


void FixtureNode::setFlag(const std::string &flag, bool value)
{
   mFlags[flag]=value;
}


bool FixtureNode::hasFlag(const std::string &flag)
{
   bool value = mFlags[flag];
   return value;
}


void FixtureNode::setProperty(const std::string &key, const Variant& value)
{
    mProperties[key] = value;
}


FixtureNode::Variant FixtureNode::getProperty(const std::string& key) const
{
   return mProperties.find(key)->second;
}

