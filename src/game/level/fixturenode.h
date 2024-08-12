#pragma once

#include "constants.h"
#include "gamenode.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

class FixtureNode : public GameNode
{
public:
   using CollisionCallback = std::function<void(void)>;
   using Variant = std::variant<std::string, int32_t, double>;

   FixtureNode(GameNode* parent);

   ObjectType getType() const;
   void setType(const ObjectType& type);

   void setFlag(const std::string& flag, bool value);
   bool hasFlag(const std::string& flag);

   void setProperty(const std::string& key, const Variant& value);
   Variant getProperty(const std::string& key) const;
   bool hasProperty(const std::string& key) const;

   virtual void collisionWithPlayer();
   void setCollisionCallback(const CollisionCallback& collisionCallback);

protected:
   ObjectType _type = ObjectTypeInvalid;
   std::map<std::string, bool> _flags;
   std::map<std::string, Variant> _properties;
   CollisionCallback _collision_callback;
};
